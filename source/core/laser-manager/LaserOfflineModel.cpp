#include "LaserModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <mining-manager/MiningManager.h>
#include <glog/logging.h>

namespace sf1r { namespace laser {

class OfflineStable
{
public:
    float betaStable() const
    {
        return betaStable_;
    }

    const std::vector<float>& conjunctionStable() const
    {
        return conjunctionStable_;
    }
    MSGPACK_DEFINE(betaStable_, conjunctionStable_);
private:
    float betaStable_;
    std::vector<float> conjunctionStable_;
};

LaserOfflineModel::LaserOfflineModel(const AdIndexManager& adIndexer,
    const std::string& filename,
    const std::string& sysdir,
    const std::size_t adDimension)
    : adIndexer_(adIndexer)
    , filename_(filename)
    , sysdir_(sysdir)
    , adDimension_(adDimension)
    , alpha_(NULL)
    , beta_(NULL)
    , betaStable_(NULL)
    , conjunction_(NULL)
    , conjunctionStable_(NULL)
    , threadGroup_(NULL)
    , origBetaStable_(NULL)
    , origConjunctionStable_(NULL)
{
    const std::string betaDB = sysdir_ + "/beta-stable-offline-model";
    if (!boost::filesystem::exists(betaDB))
    {
        boost::filesystem::create_directories(betaDB);
    }
    origBetaStable_ = new OrigBetaStableDB(betaDB);
    
    const std::string conjunctionDB = sysdir_ + "/conjunction-stable-offline-model";
    if (!boost::filesystem::exists(conjunctionDB))
    {
        boost::filesystem::create_directories(conjunctionDB);
    }
    origConjunctionStable_ = new OrigConjunctionStableDB(conjunctionDB);
    
    alpha_ = new std::vector<float>();
    beta_ = new std::vector<float>();
    betaStable_ = new std::vector<float>(adDimension_);
    conjunction_ = new std::vector<std::vector<float> >();
    conjunctionStable_ = new std::vector<std::vector<float> >(adDimension_);
    if (boost::filesystem::exists(filename_))
    {
        load();
    }
    else
    {
        LOG(INFO)<<"init per-item-online-model from original model, since localized model doesn't exist. \
            This procedure may be slow, be patient";
        localizeFromOrigModel();
    }
        /*std::size_t adf = 10000;
        std::size_t userf = 200;
        alpha_->resize(userf);
        beta_->resize(adf);
        conjunction_->resize(userf);
        for (std::size_t i = 0; i < adDimension_; ++i)
        {
            (*betaStable_)[i] = (rand() % 100) / 100.0;
            std::vector<float> vec;
            for (std::size_t k = 0; k < userf; ++k)
            {
                vec.push_back((rand() % 100) / 100.0);
            }
            (*conjunctionStable_)[i] = vec;
        }
        for (std::size_t i = 0; i < userf; ++i)
        {
            (*alpha_)[i] = (rand() % 100) / 100.0;
        }
        for (std::size_t i = 0; i < adf; ++i)
        {
            (*beta_)[i] = (rand() % 100) / 100.0;
        }
        for (std::size_t i = 0; i < userf; ++i)
        {
            std::vector<float> vec;
            for (std::size_t k = 0; k < adf; k++)
            {
                vec.push_back((rand() % 100) / 100.0);
            }
            (*conjunctionStable_)[i] = vec;
        }
        save();*/
}

LaserOfflineModel::~LaserOfflineModel()
{
    if (NULL != threadGroup_)
    {
        threadGroup_->join_all();
        delete threadGroup_;
    }
    if (NULL != alpha_)
    {
        delete alpha_;
        alpha_ = NULL;
    }
    if (NULL != beta_)
    {
        delete beta_;
        beta_ = NULL;
    }
    if (NULL != betaStable_)
    {
        delete betaStable_;
        betaStable_ = NULL;
    }
    if (NULL != conjunction_)
    {
        delete conjunction_;
        conjunction_ = NULL;
    }
    if (NULL != conjunctionStable_)
    {
        delete conjunctionStable_;
        conjunctionStable_ = NULL;
    }
    if (NULL != origBetaStable_)
    {
        delete origBetaStable_;
        origBetaStable_ = NULL;
    }
    if (NULL != origConjunctionStable_)
    {
        delete origConjunctionStable_;
        origConjunctionStable_ = NULL;
    }
}

float LaserOfflineModel::score( 
    const std::string& text,
    const std::vector<std::pair<int, float> >& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score; 
    // x * alpha
    ret += dot(*alpha_, user);
    
    // c * beta
    if (ad.first < betaStable_->size())
    {
        ret += (*betaStable_)[ad.first];
    }
    else
    {
        return ret;
    }
    
    // x * A * c
    if (ad.first < conjunctionStable_->size())
    {
        ret += dot((*conjunctionStable_)[ad.first], user);
    }
    else
    {
        return ret;
    }
    return ret;
}

float LaserOfflineModel::score( 
    const std::string& text,
    const std::vector<float>& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score; 
    // x * alpha
    ret += dot(*alpha_, user);
    
    // c * beta
    if (ad.first < betaStable_->size())
    {
        ret += (*betaStable_)[ad.first];
    }
    else
    {
        return ret;
    }
    
    // x * A * c
    if (ad.first < conjunctionStable_->size())
    {
        ret += dot((*conjunctionStable_)[ad.first], user);
    }
    else
    {
        return ret;
    }
    return ret;
}

void LaserOfflineModel::dispatch(const std::string& method, msgpack::rpc::request& req)
{
    if ("precompute_ad_offline_model" == method)
    {
        msgpack::type::tuple<std::string, OfflineStable> params;
        req.params().convert(&params);
        const std::string& DOCID = params.get<0>();
        LOG(INFO)<<params.get<0>();
        const OfflineStable& stable = params.get<1>();
        origBetaStable_->update(DOCID, stable.betaStable());
        origConjunctionStable_->update(DOCID, stable.conjunctionStable());
        docid_t adid = 0;
        if (adIndexer_.convertDocId(DOCID, adid))
        {
            (*betaStable_)[adid] = stable.betaStable();
            (*conjunctionStable_)[adid] = stable.conjunctionStable();
        }
        req.result(true);
    }
    else if ("updateOfflineModel" == method)
    {
        LOG(INFO)<<"update OfflineModel ..."; 
        msgpack::type::tuple<std::vector<float>, std::vector<float>, std::vector<std::vector<float> > > params;
        req.params().convert(&params);
        *alpha_ = params.get<0>();
        *beta_ = params.get<1>();
        *conjunction_ = params.get<2>();
        req.result(true);
    }
    else if ("finish_offline_model" == method)
    {
        save();
        saveOrigModel();
        req.result(true);
    }
    else
    {
        req.error(msgpack::rpc::NO_METHOD_ERROR);
    }
}

void LaserOfflineModel::save()
{
    std::ofstream ofs(filename_.c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << alpha_;
        oa << beta_;
        oa << beta_;
        oa << conjunction_;
        oa << conjunctionStable_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}

void LaserOfflineModel::load()
{
    std::ifstream ifs(filename_.c_str(), std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
    try
    {
        ia >> alpha_;
        ia >> beta_;
        ia >> betaStable_;
        ia >> conjunction_;
        ia >> conjunctionStable_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ifs.close();
}
    
void LaserOfflineModel::updateAdDimension(const std::size_t adDimension)
{
    betaStable_->resize(adDimension);
    conjunctionStable_->resize(adDimension);
    if (NULL != threadGroup_)
    {
        LOG(INFO)<<"wait last time's pre-compute threads finish";
        threadGroup_->join_all();
        delete threadGroup_;
    }
    threadGroup_ = new boost::thread_group();
    for (int i = 0; i < THREAD_NUM; ++i)
    {
        threadGroup_->create_thread(
            boost::bind(&LaserOfflineModel::precompute, this,
                        adDimension_, adDimension, i));
    }
    adDimension_ = adDimension;
}
    
void LaserOfflineModel::precompute(std::size_t startId, std::size_t endId, int threadId)
{
    for (std::size_t adId = startId; adId < endId; ++adId)
    {
        if (0 != adId % THREAD_NUM)
            continue;

        std::vector<std::pair<int, float> > vec;
        adIndexer_.get(adId, vec);
        (*betaStable_)[adId] = dot(*beta_, vec);
        
        std::vector<float>& row = (*conjunctionStable_)[adId];
        std::vector<std::vector<float> >::const_iterator it = conjunction_->begin();
        for (; it != conjunction_->end(); ++it)
        {
            row.push_back(dot(*it, vec));
        }
    }
}

void LaserOfflineModel::saveOrigModel()
{
    std::ofstream ofs((MiningManager::system_working_path_ + "/Laser/orig-offline-model").c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << alpha_;
        oa << beta_;
        oa << beta_;
        oa << conjunction_;
        oa << conjunctionStable_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}

void LaserOfflineModel::localizeFromOrigModel()
{
    {
        OrigBetaStableDB::iterator it = origBetaStable_->begin();
        for (; it != origBetaStable_->end(); ++it)
        {
            docid_t id = 0;
            if (adIndexer_.convertDocId(it->first, id))
            {
                (*betaStable_)[id] = it->second;
            }
        }
    }
    
    {
        OrigConjunctionStableDB::iterator it = origConjunctionStable_->begin();
        for (; it != origConjunctionStable_->end(); ++it)
        {
            docid_t id = 0;
            if (adIndexer_.convertDocId(it->first, id))
            {
                (*conjunctionStable_)[id] = it->second;
            }
        }
    }
    
    const std::string origOfflineModel = sysdir_ + "/orig-offline-model";
    if (!boost::filesystem::exists(origOfflineModel))
    {
        LOG(INFO)<<"orig offline model does't exist.";
        return;
        //throw std::runtime_error("orig offline model does't exist");
    }
    std::ifstream ifs(origOfflineModel.c_str(), std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
    try
    {
        ia >> *alpha_;
        ia >> *beta_;
        ia >> *conjunction_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ifs.close();
}
    
} }
