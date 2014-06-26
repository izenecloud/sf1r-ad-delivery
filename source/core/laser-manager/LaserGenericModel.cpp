#include "LaserGenericModel.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include "context/KVClient.h"
#include "context/MQClient.h"

namespace sf1r { namespace laser {

LaserGenericModel::LaserGenericModel(const AdIndexManager& adIndexer, 
    const std::string& kvaddr,
    const int kvport,
    const std::string& mqaddr,
    const int mqport,
    const std::string& workdir)
    : adIndexer_(adIndexer)
    , workdir_(workdir)
    , pAdDb_(NULL)
    , offlineModel_(NULL)
    , kvclient_(NULL)
    , mqclient_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    pAdDb_ = new LaserModelDB<docid_t, LaserOnlineModel>(workdir_ + "/per-item-online-model");
    offlineModel_ = new LaserOfflineModel(workdir_ + "/offline-model");
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

float LaserGenericModel::score(
    const std::string& text,
    const std::vector<std::pair<int, float> >& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    LaserOnlineModel onlineModel;
    if (!pAdDb_->get(ad.first, onlineModel))
    {
        return ret;
    }

    static const std::pair<docid_t, std::vector<std::pair<int, float> > > perAd;
    ret += onlineModel.score(text, user, perAd, 0);
    {
        boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
        if (sharedLock)
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
}
    
void LaserGenericModel::updatepAdDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, float, std::vector<float> > params;
    req.params().convert(&params);
    LaserOnlineModel onlineModel(params.get<1>(), params.get<2>());
    const std::string id(params.get<0>());
    std::stringstream ss(id); 
    docid_t docid = 0;
    ss >> docid;
    bool res = pAdDb_->update(docid, onlineModel);
    req.result(res);
}

void LaserGenericModel::updateOfflineModel(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::vector<float>, std::vector<float>, std::vector<std::vector<float> > > params;
    req.params().convert(&params);
    std::vector<float> alpha(params.get<0>());
    std::vector<float> beta(params.get<1>());
    std::vector<std::vector<float> > quadratic(params.get<2>());
    
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    offlineModel_->setAlpha(alpha);
    offlineModel_->setBeta(beta);
    offlineModel_->setQuadratic(quadratic);
    offlineModel_->save();
    req.result(true);
}

} }
