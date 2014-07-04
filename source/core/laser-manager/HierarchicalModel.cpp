#include "HierarchicalModel.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include <util/functional.h>
#include <algorithm>    // std::sort
    
namespace sf1r { namespace laser {

HierarchicalModel::HierarchicalModel(const AdIndexManager& adIndexer, 
    const std::string& kvaddr,
    const int kvport,
    const std::string& mqaddr,
    const int mqport,
    const std::string& workdir,
    const std::size_t adDimension,
    const std::size_t clusteringDimension)
    : LaserGenericModel(adIndexer, kvaddr, kvport, mqaddr, mqport, workdir, adDimension)
    , workdir_(workdir)
    , clusteringDimension_(clusteringDimension)
    , pClusteringDb_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    pClusteringDb_ = new std::vector<LaserOnlineModel>(clusteringDimension_);
    LOG(INFO)<<"clustering dimension = "<<clusteringDimension_;
    if (boost::filesystem::exists(workdir_ + "per-clustering-online-model"))
    {
        load();
    }
    LOG(INFO)<<"per-clustering-model size = "<<pClusteringDb_->size();
    /*
    for (std::size_t i = 0; i < clusteringDimension_; ++i)
    {
        float delta = (rand() % 100) / 100.0;
        std::vector<float> eta(200);
        for (std::size_t k = 0; k < 200; ++k)
        {
            eta[k] = (rand() % 100) / 100.0;
        }
        LaserOnlineModel onlineModel(delta, eta);
        (*pClusteringDb_)[i] = onlineModel;
    }
    */
    save();
}

HierarchicalModel::~HierarchicalModel()
{
    if (NULL != pClusteringDb_)
    {
        delete pClusteringDb_;
        pClusteringDb_ = NULL;
    }
}

bool HierarchicalModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    boost::posix_time::ptime stime = boost::posix_time::microsec_clock::local_time();
    typedef izenelib::util::second_greater<std::pair<std::size_t, float> > greater_than;
    typedef std::priority_queue<std::pair<size_t, float>, std::vector<std::pair<std::size_t, float> >, greater_than> priority_queue;
    priority_queue queue;
    for (std::size_t i = 0; i < clusteringDimension_; ++i)
    {
        static const std::pair<docid_t, std::vector<std::pair<int, float> > > perAd;
        float score = (*pClusteringDb_)[i].score(text, context, perAd, (float)0.0);
        if (queue.size() < 1024)
        {
            queue.push(std::make_pair(i, score));
        }
        else
        {
            if (score > queue.top().second)
            {
                queue.pop();
                queue.push(std::make_pair(i, score));
            }
        }
    }
    std::vector<std::pair<std::size_t, float> > clustering;
    clustering.reserve(queue.size());
    while (!queue.empty())
    {
        clustering.push_back(queue.top());
        queue.pop();
    }
    boost::posix_time::ptime etime = boost::posix_time::microsec_clock::local_time();
    LOG(INFO)<<"score time = "<<(etime-stime).total_milliseconds();
    
    stime = boost::posix_time::microsec_clock::local_time();
    for(int i = clustering.size() - 1; i >= 0; --i)
    {
        if (!adIndexer_.get(clustering[i].first, ad))
        {
            LOG(ERROR)<<"clustering : "<<clustering[i].first<<" container no ad";
        }
        if (ad.size() >= ncandidate)
            break;
    }
    etime = boost::posix_time::microsec_clock::local_time();
    LOG(INFO)<<"query time = "<<(etime-stime).total_milliseconds();
    score.assign(ad.size(), 0);
    return true;
}

void HierarchicalModel::dispatch(const std::string& method, msgpack::rpc::request& req)
{
    if ("updatePerClusteringModel" == method)
    {
        updatepClusteringDb(req);
        return;
    }
    if ("finish_online_model" == method)
    {
        save();
    }
    LaserGenericModel::dispatch(method, req);
}

void HierarchicalModel::updatepClusteringDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, LaserOnlineModel> params;
    //LOG(INFO)<<"TODO convert"<<req.params();
    req.params().convert(&params);
    const std::string id(params.get<0>());
    std::size_t pos = id.find("|clustering");
    if (std::string::npos == pos)
    {
        req.error(msgpack::rpc::ARGUMENT_ERROR);
        return;
    }
    std::stringstream ss(id.substr(0, pos)); 
    std::size_t clusteringId = 0;
    ss >> clusteringId;
    //LOG(INFO)<<clusteringId;
    (*pClusteringDb_)[clusteringId] =  params.get<1>();
    req.result(true);
}

void HierarchicalModel::save()
{
    std::ofstream ofs((workdir_ + "per-clustering-online-model").c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << *pClusteringDb_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}

void HierarchicalModel::load()
{
    std::ifstream ifs((workdir_ + "per-clustering-online-model").c_str(), std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
    try
    {
        ia >> *pClusteringDb_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ifs.close();
}
    
    
} }
