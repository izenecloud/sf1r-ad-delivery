#include "LaserGenericModel.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include "SparseVector.h"
#include "context/KVClient.h"
#include "context/MQClient.h"

#include <mining-manager/MiningManager.h>

namespace sf1r { namespace laser {


class AdFeature
{
public:
    long& adId() 
    {
        return adId_;
    }

    std::vector<int>& index()
    {
        return vector_.index();
    }

    std::vector<float>& value()
    {
        return vector_.value();
    }
    
    MSGPACK_DEFINE(adId_, vector_);
private:
    long adId_;
    SparseVector vector_;
};

LaserGenericModel::LaserGenericModel(const AdIndexManager& adIndexer, 
    const std::string& kvaddr,
    const int kvport,
    const std::string& mqaddr,
    const int mqport,
    const std::string& workdir,
    const std::string& sysdir,
    const std::size_t adDimension,
    const std::size_t AD_FD,
    const std::size_t USER_FD)
    : adIndexer_(adIndexer)
    , workdir_(workdir)
    , sysdir_(sysdir)
    , adDimension_(adDimension)
    , AD_FD_(AD_FD)
    , USER_FD_(USER_FD)
    , pAdDb_(NULL)
    , offlineModel_(NULL)
    , kvclient_(NULL)
    , mqclient_(NULL)
    , origLaserModel_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    const std::string DB = sysdir_ + "/orig-per-item-online-model";
    if (!boost::filesystem::exists(DB))
    {
        boost::filesystem::create_directories(DB);
    }
    origLaserModel_ = new OrigOnlineDB(DB);
    
    LOG(INFO)<<"open per-item-online-model...";
    LOG(INFO)<<"AD Feature Dimension = "<<AD_FD_<<", USER Feature Dimension = "<<USER_FD_;
    if (boost::filesystem::exists(workdir_ + "/per-item-online-model"))
    {
        pAdDb_ = new std::vector<LaserOnlineModel>();
        load();
    }
    else
    {
        std::vector<float> vec(USER_FD_);
        LaserOnlineModel initModel(0.0, vec);
        pAdDb_ = new std::vector<LaserOnlineModel>(adDimension_, initModel);
        LOG(INFO)<<"init per-item-online-model from OrigModel, since localized model doesn't exist. \
            This procedure may be slow, be patient";
        localizeFromOrigModel();
        LOG(INFO)<<"save model to local";
        save();
    }
    LOG(INFO)<<"per-item-online-model size = "<<pAdDb_->size();
    /*
    std::vector<float> vec(USER_FD_);
    LaserOnlineModel initModel(0.0, vec);
    pAdDb_ = new std::vector<LaserOnlineModel>(adDimension_, initModel);
    LOG(INFO)<<"per-item-online-model size = "<<pAdDb_->size();
    for (std::size_t i = 0; i < adDimension_; ++i)
    {
        float delta = (rand() % 100) / 100.0;
        std::vector<float> eta(200);
        for (std::size_t k = 0; k < 200; ++k)
        {
            eta[k] = (rand() % 100) / 100.0;
        }
        LaserOnlineModel onlineModel(delta, eta);
        (*pAdDb_)[i] = onlineModel;
    }
    save();
    */

    LOG(INFO)<<"open offline-model";
    offlineModel_ = new LaserOfflineModel(adIndexer_, workdir_ + "/offline-model", sysdir_, adDimension_, AD_FD_, USER_FD_);
    
    kvclient_ = new context::KVClient(kvaddr, kvport);
    mqclient_ = new context::MQClient(mqaddr, mqport);

}

LaserGenericModel::~LaserGenericModel()
{
    if (NULL != origLaserModel_)
    {
        delete origLaserModel_;
        origLaserModel_ = NULL;
    }
    if (NULL != pAdDb_)
    {
        delete pAdDb_;
        pAdDb_ = NULL;
    }
    if (NULL != offlineModel_)
    {
        delete offlineModel_;
        offlineModel_ = NULL;
    }
    if (NULL != kvclient_)
    {
        kvclient_->shutdown();
        delete kvclient_;
        kvclient_ = NULL;
    }
    if (NULL != mqclient_)
    {
        mqclient_->shutdown();
        delete mqclient_;
        mqclient_ = NULL;
    }
}
    
void LaserGenericModel::updateAdDimension(const std::size_t adDimension)
{
    adDimension_ = adDimension;
    offlineModel_->updateAdDimension(adDimension_);
    pAdDb_->resize(adDimension_);
}
    


bool LaserGenericModel::context(const std::string& text, 
     std::vector<std::pair<int, float> >& context) const
{
    bool ret = kvclient_->context(text, context);
    if (!ret)
    {
        mqclient_->publish(text);
    }
    return ret;
}

bool LaserGenericModel::context(const std::string& text, 
     std::vector<float>& context) const
{
    bool ret = kvclient_->context(text, context);
    if (!ret)
    {
        mqclient_->publish(text);
    }
    return ret;
}

bool LaserGenericModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    const docid_t& lastDocId = adIndexer_.getLastDocId();
    for (docid_t docid = 0; docid < lastDocId; ++docid)
    {
        if (adIndexer_.get(docid, ad))
        {
            score.push_back(0);
        }
    }
    return true;
}

bool LaserGenericModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<float>& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    const docid_t& lastDocId = adIndexer_.getLastDocId();
    for (docid_t docid = 0; docid < lastDocId; ++docid)
    {
        if (adIndexer_.get(docid, ad))
        {
            score.push_back(0);
        }
    }
    return true;
}

float LaserGenericModel::score(
    const std::string& text,
    const std::vector<std::pair<int, float> >& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    if (ad.first > adDimension_)
    {
        return ret;
    }
    static const std::pair<docid_t, std::vector<std::pair<int, float> > > perAd;
    ret += (*pAdDb_)[ad.first].score(text, user, perAd, 0);
    {
        ret += offlineModel_->score(text, user, ad, 0);
    }
    return ret;
}

float LaserGenericModel::score(
    const std::string& text,
    const std::vector<float>& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    if (ad.first > adDimension_)
    {
        return ret;
    }
    static const std::pair<docid_t, std::vector<std::pair<int, float> > > perAd;
    ret += (*pAdDb_)[ad.first].score(text, user, perAd, 0);
    {
        ret += offlineModel_->score(text, user, ad, 0);
    }
    return ret;
}
    
void LaserGenericModel::dispatch(const std::string& method, msgpack::rpc::request& req)
{
    if ("update_online_model" == method)
    {
        updatepAdDb(req);
    }
    else if ("ad_feature|size" == method)
    {
        LOG(INFO)<<"send ad dimension: "<<adDimension_;
        req.result(adDimension_);
    }
    else if ("ad_feature|next" == method)
    {
        AdFeature ad;
        ad.adId() = adIndex_;

        std::vector<std::pair<int, float> > vec;
        docid_t docid = adIndex_;
        adIndexer_.get(docid, vec);
        std::vector<int>& index = ad.index();
        std::vector<float>& value = ad.value();
        for (std::size_t k = 0; k < vec.size(); ++k)
        {
            index.push_back(vec[k].first);
            value.push_back(vec[k].second);
        }

        req.result(ad);
        adIndex_++;
    }
    else if ("ad_feature|start" == method)
    {
        msgpack::type::tuple<long> params;
        req.params().convert(&params);
        adIndex_ = params.get<0>();
        LOG(INFO)<<"begin iterate: "<<adIndex_;
        req.result(true);
    }
    else if ("finish_online_model" == method)
    {
        LOG(INFO)<<"save online model";
        save();
    }
    else
    {
        offlineModel_->dispatch(method, req);
    }
}
    
void LaserGenericModel::updatepAdDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, LaserOnlineModel> params;
    req.params().convert(&params);
    const std::string DOCID(params.get<0>());
    const LaserOnlineModel& model = params.get<1>();
    if (model.eta().size() != USER_FD_)
    {
        LOG(ERROR)<<"Dimension Mismatch";
        req.result(false);
        return;
    }

    origLaserModel_->update(DOCID, model);
    docid_t adid = 0;
    if (adIndexer_.convertDocId(DOCID, adid))
    {
        (*pAdDb_)[adid] = model;
    }
    req.result(true);
}
    
void LaserGenericModel::load()
{
    std::ifstream ifs((workdir_ + "per-item-online-model").c_str(), std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    try
    {
        ia >> *pAdDb_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ifs.close();
}

void LaserGenericModel::save()
{
    std::ofstream ofs((workdir_ + "per-item-online-model").c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::binary_oarchive oa(ofs);
    try
    {
        oa << *pAdDb_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}
    
void LaserGenericModel::localizeFromOrigModel()
{
    OrigOnlineDB::iterator it = origLaserModel_->begin();
    for (; it != origLaserModel_->end(); ++it)
    {
        docid_t id = 0;
        if (adIndexer_.convertDocId(it->first, id))
        {
            (*pAdDb_)[id] = it->second;
        }
    }
}

} }
