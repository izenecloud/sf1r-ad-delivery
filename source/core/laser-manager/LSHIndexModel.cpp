#include "LSHIndexModel.h"
#include "AdIndexManager.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include <math.h>

namespace sf1r { namespace laser {

static const int H = 1017881;   // default hash table size
static const float P1 = 0.8;
static const float P2 = 0.01;
static const float DELTA = 1e-4;
static const int ALSH_M = 3;    // default M value in ALSH
static const float ALSH_W = 2.5;
static const std::size_t THREAD_NUM = 16;

LSHIndexModel::LSHIndexModel(const AdIndexManager& adIndexer, 
    const std::string& kvaddr,
    const int kvport,
    const std::string& mqaddr,
    const int mqport,
    const std::string& workdir,
    const std::string& sysdir,
    const std::size_t adDimension,
    const std::size_t AD_FD,
    const std::size_t USER_FD)
    : LaserGenericModel(adIndexer, kvaddr, kvport, mqaddr, mqport, workdir, sysdir, adDimension, AD_FD, USER_FD)
    , workdir_(workdir)
    , lsh_(NULL)
    , LSH_DIM_(1 + USER_FD_ + USER_FD_ + 1 + USER_FD_)
    , ALSH_DIM_(LSH_DIM_ + ALSH_M)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    
    const std::string lsh = workdir_ + "/lsh-index-model";
    if (boost::filesystem::exists(lsh))
    {
        lsh_ = new LshIndex();
        load();
    }
    else
    {
        lsh_ = createLshIndex();
    }
    //lsh_ = createLshIndex();
    //buildLshIndex(lsh_);
    //save();
}

LSHIndexModel::~LSHIndexModel()
{
    if (NULL != lsh_)
    {
        delete lsh_;
        lsh_ = NULL;
    }
}

bool LSHIndexModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    //TODO
    return true;
}

bool LSHIndexModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<float>& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    SCANNER scanner(adDimension_, ad);
    float* query = new float[ALSH_DIM_];
    float* thisRow = query;
    *thisRow = 1.0;
    ++thisRow;
    memcpy(thisRow, context.data(), USER_FD_);
    thisRow += USER_FD_;

    memcpy(thisRow, context.data(), USER_FD_);
    thisRow += USER_FD_;
    *thisRow = 1.0;
    ++thisRow;
    memcpy(thisRow, context.data(), USER_FD_);
    thisRow += USER_FD_;


    for (int k = 0; k < ALSH_M; ++k)
    {
        *thisRow = 0.5;
        ++thisRow;
    }
    boost::shared_lock<boost::shared_mutex> sharedLock(mtx_, boost::try_to_lock);
    if (!sharedLock)
    {
        delete query;
        return false;
    }
    lsh_->query(query, scanner);
    delete query;
    score.assign(ad.size(), 0);
    return true;
}

void LSHIndexModel::dispatch(const std::string& method, msgpack::rpc::request& req)
{
    LaserGenericModel::dispatch(method, req);
    if ("finish_online_model" == method || "finish_offline_model" == method)
    {
        LshIndex* lsh = createLshIndex();
        buildLshIndex(lsh);
        { 
            boost::unique_lock<boost::shared_mutex> uniqueLock(mtx_);
            std::swap(lsh_, lsh);
        }
        delete lsh;
        save();
    }

}

LSHIndexModel::LshIndex* LSHIndexModel::createLshIndex()
{
    static const int M = adDimension_ > 1 ? log(adDimension_) / log(1 / P2) : 1;         
    static const int L = ceil(1.5 * log(DELTA) / log(1 - exp(M * log(P1))));        // default hash table number
    static LshIndex::Parameter param;
    LOG(INFO)<<"LSH parameter, M = "<<M<<", L = "<<L<<", DIM = "<<ALSH_DIM_;
    param.range = H;
    param.repeat = M;
    param.dim = ALSH_DIM_;
    param.W = ALSH_W;
    lshkit::DefaultRng rng;
    LshIndex* lsh = new LshIndex();
    lsh->init(param, rng, L);
    return lsh;
}

void LSHIndexModel::buildLshIndex(LshIndex* lsh)
{
    LOG(INFO)<<"build LSH Index...";
    boost::posix_time::ptime stime = boost::posix_time::microsec_clock::local_time();
    boost::thread_group threadGroup;
    for (std::size_t i = 0; i < THREAD_NUM; ++i)
    {
        threadGroup.create_thread(
            boost::bind(&LSHIndexModel::buildLshIndex, this, lsh, i));
    }
    threadGroup.join_all();
    LOG(INFO)<<"LSH Index finished...";
    boost::posix_time::ptime etime = boost::posix_time::microsec_clock::local_time();
    LOG(INFO)<<"index time = "<<(etime-stime).total_milliseconds();
}

void LSHIndexModel::buildLshIndex(LshIndex* lsh, std::size_t threadId)
{
    const std::vector<LaserOnlineModel>* data = pAdDb_;
    const std::vector<float>* alpha = offlineModel_->alpha();
    const std::vector<float>* betaStable = offlineModel_->betaStable();
    const std::vector<std::vector<float> >* conjunctionStable = offlineModel_->conjunctionStable();
    if (NULL == data || data->empty())
        return;

    float* row = new float[ALSH_DIM_]; 
    for (std::size_t i = 0; i < adDimension_; ++i)
    {
        if (i % THREAD_NUM!= threadId)
            continue;
        float* thisRow = row;
        *thisRow = (*data)[i].delta();
        ++thisRow;
        memcpy(thisRow, (*data)[i].eta().data(), USER_FD_);
        thisRow += USER_FD_;

        memcpy(thisRow, alpha->data(), USER_FD_);
        thisRow += USER_FD_;

        *thisRow = (*betaStable)[i];
        ++thisRow;
        memcpy(thisRow, (*conjunctionStable)[i].data(), USER_FD_);
        thisRow += USER_FD_;

        float norm = dot(row, row, LSH_DIM_);
        *thisRow = norm;
        ++thisRow;

        for (int k = 1; k < ALSH_M; ++k)
        {
            *thisRow = thisRow[-1] * thisRow[-1];
            ++thisRow;
        }
        lsh->insert(i, row);
    }
    delete row;
}

void LSHIndexModel::save()
{
    std::ofstream ofs((workdir_ + "/lsh-index-model").c_str(), std::ofstream::binary | std::ofstream::trunc);
    lsh_->save(ofs);
    ofs.close();
}

void LSHIndexModel::load()
{
    std::ifstream ifs((workdir_ + "/lsh-index-model").c_str(), std::ios::binary);
    lsh_->load(ifs);
    ifs.close();
}
    
} }
