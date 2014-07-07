#include "LaserGenericModel.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include "SparseVector.h"
#include "context/KVClient.h"
#include "context/MQClient.h"

namespace sf1r { namespace laser {

class OfflineStable
{
public:
    float betaStable() const
    {
        return betaStable_;
    }

    const std::vector<float>& quadraticStable() const
    {
        return quadraticStable_;
    }
    MSGPACK_DEFINE(betaStable_, quadraticStable_);
private:
    float betaStable_;
    std::vector<float> quadraticStable_;
};

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
    const std::size_t adDimension)
    : adIndexer_(adIndexer)
    , workdir_(workdir)
    , adDimension_(adDimension)
    , pAdDb_(NULL)
    , offlineModel_(NULL)
    , kvclient_(NULL)
    , mqclient_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    LOG(INFO)<<"open per-item-online-model...";
    pAdDb_ = new std::vector<LaserOnlineModel>();
    LOG(INFO)<<"ad dimension = "<<adDimension_;
    if (boost::filesystem::exists(workdir_ + "per-item-online-model"))
    {
        load();
    }
    LOG(INFO)<<"per-item-online-model size = "<<pAdDb_->size();
    if (pAdDb_->size() < adDimension_)
    {
        pAdDb_->resize(adDimension_);
    }
    /*
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
    offlineModel_ = new LaserOfflineModel(adIndexer_, workdir_ + "/offline-model", adDimension_);
    
    kvclient_ = new context::KVClient(kvaddr, kvport);
    mqclient_ = new context::MQClient(mqaddr, mqport);
}

LaserGenericModel::~LaserGenericModel()
{
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
    if ("updatePerAdOnlineModel" == method)
    {
        updatepAdDb(req);
    }
    else if ("updateOfflineModel" == method)
    {
        updateOfflineModel(req);
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
        adIndex_++;

        std::vector<int>& index = ad.index();
        std::vector<float>& value = ad.value();
        for (int k = 0; k < 10; ++k)
        {
            index.push_back(rand() % 10000);
            value.push_back((rand() % 100) / 100.0);
        }

        req.result(ad);
    }
    else if ("ad_feature|start" == method)
    {
        msgpack::type::tuple<long> params;
        req.params().convert(&params);
        adIndex_ = params.get<0>();
        LOG(INFO)<<"begin iterate: "<<adIndex_;
        req.result(true);
    }
    else if ("precompute_ad_offline_model" == method)
    {
        msgpack::type::tuple<long, OfflineStable> params;
        req.params().convert(&params);
        LOG(INFO)<<params.get<0>();
        req.result(true);
    }
    else if ("finish_offline_model" == method)
    {
        offlineModel_->save();
    }
    else if ("finish_online_model" == method)
    {
        LOG(INFO)<<"save online model";
        save();
    }
}
    
void LaserGenericModel::updatepAdDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, LaserOnlineModel> params;
    req.params().convert(&params);
    const std::string id(params.get<0>());
    // TODO DOCID => docid_t
    std::stringstream ss(id); 
    docid_t docid = 0;
    ss >> docid;
    LOG(INFO)<<docid;
    (*pAdDb_)[docid] =  params.get<1>();
    req.result(true);
}

void LaserGenericModel::updateOfflineModel(msgpack::rpc::request& req)
{
    LOG(INFO)<<"update OfflineModel ..."; 
    msgpack::type::tuple<std::vector<float>, std::vector<float>, std::vector<std::vector<float> > > params;
    req.params().convert(&params);
    std::vector<float> alpha(params.get<0>());
    std::vector<float> beta(params.get<1>());
    std::vector<std::vector<float> > quadratic(params.get<2>());
    offlineModel_->setAlpha(alpha);
    offlineModel_->setBeta(beta);
    offlineModel_->setQuadratic(quadratic);
    req.result(true);
}
    
void LaserGenericModel::load()
{
    std::ifstream ifs((workdir_ + "per-item-online-model").c_str(), std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
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
    boost::archive::text_oarchive oa(ofs);
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

} }
