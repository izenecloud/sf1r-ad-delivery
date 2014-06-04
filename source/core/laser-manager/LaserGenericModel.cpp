#include "LaserGenericModel.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"

namespace sf1r { namespace laser {

LaserGenericModel::LaserGenericModel(const AdIndexManager& adIndexer, const std::string& workdir)
    : adIndexer_(adIndexer)
    , workdir_(workdir)
    , pAdDb_(NULL)
    , offlineModel_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    pAdDb_ = new LaserModelDB<docid_t, LaserOnlineModel>(workdir_ + "/per-item-online-model");
    offlineModel_ = new LaserOfflineModel(workdir_ + "/offline-model");
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
}

bool LaserGenericModel::candidate(
    const std::string& text,
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
    }
    else if ("updateOfflineModel" == method)
    {
    }
}
    
void LaserGenericModel::updatepAdDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, std::string, float, std::vector<float> > params;
    req.params().convert(&params);
    LaserOnlineModel onlineModel(params.get<2>(), params.get<3>());
    const std::string id(params.get<1>());
    // TODO convert
    docid_t docid = 0;
    bool res = pAdDb_->update(docid, onlineModel);
    req.result(res);
}

void LaserGenericModel::updateOfflineModel(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, std::vector<float>, std::vector<float>, std::vector<std::vector<float> > > params;
    std::vector<float> alpha(params.get<1>());
    std::vector<float> beta(params.get<2>());
    std::vector<std::vector<float> > quadratic(params.get<3>());
    
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    offlineModel_->setAlpha(alpha);
    offlineModel_->setBeta(beta);
    offlineModel_->setQuadratic(quadratic);
}

} }
