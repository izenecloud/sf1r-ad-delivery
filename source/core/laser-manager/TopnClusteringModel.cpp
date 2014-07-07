#include "TopnClusteringModel.h"
#include "TopNClusteringDB.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"

namespace sf1r { namespace laser {
TopnClusteringModel::TopnClusteringModel(const AdIndexManager& adIndexer,
    const std::vector<std::vector<int> >& similarClustering,
    const std::string& workdir)
    : adIndexer_(adIndexer)
    , similarClustering_(similarClustering)
    , workdir_(workdir)
    , pUserDb_(NULL)
    , topClusteringDb_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    pUserDb_ = new LaserModelDB<std::string, LaserOnlineModel>(workdir_ + "/per-user-online-model");
    topClusteringDb_ = new TopNClusteringDB(workdir_ + "/per-user-topn-clustering", similarClustering_.size());
}

TopnClusteringModel::~TopnClusteringModel()
{
    if (NULL != pUserDb_)
    {
        delete pUserDb_;
        pUserDb_ = NULL;
    }
    if (NULL != topClusteringDb_)
    {
        delete topClusteringDb_;
        topClusteringDb_ = NULL;
    }
}

bool TopnClusteringModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    ad.reserve(ncandidate);
    score.reserve(ncandidate);

    std::map<int, float> topn;
    if (!topClusteringDb_->get(text, topn))
    {
        return false;
    }
    std::map<int, float>::const_iterator it = topn.begin();
    std::size_t old = 0;
    for (; it != topn.end(); ++it)
    {
        getAD(it->first, ad);
        for (std::size_t i = 0; i < ad.size() - old; ++i)
        {
            score.push_back(it->second);
        }
        old = ad.size();
        if (ad.size() >= ncandidate)
            return true;
    }
    
    for (it = topn.begin(); it != topn.end(); ++it)
    {
        getSimilarAd(it->first, ad);
        for (std::size_t i = 0; i < ad.size() - old; ++i)
        {
            score.push_back(it->second);
        }
        old = ad.size();
        if (ad.size() >= ncandidate)
            return true;
    }
    return true;
}

bool TopnClusteringModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<float>& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    ad.reserve(ncandidate);
    score.reserve(ncandidate);

    std::map<int, float> topn;
    if (!topClusteringDb_->get(text, topn))
    {
        return false;
    }
    std::map<int, float>::const_iterator it = topn.begin();
    std::size_t old = 0;
    for (; it != topn.end(); ++it)
    {
        getAD(it->first, ad);
        for (std::size_t i = 0; i < ad.size() - old; ++i)
        {
            score.push_back(it->second);
        }
        old = ad.size();
        if (ad.size() >= ncandidate)
            return true;
    }
    
    for (it = topn.begin(); it != topn.end(); ++it)
    {
        getSimilarAd(it->first, ad);
        for (std::size_t i = 0; i < ad.size() - old; ++i)
        {
            score.push_back(it->second);
        }
        old = ad.size();
        if (ad.size() >= ncandidate)
            return true;
    }
    return true;
}

bool TopnClusteringModel::getAD(const std::size_t& clusteringId, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad) const
{
    return adIndexer_.get(clusteringId, ad);
}

bool TopnClusteringModel::getSimilarAd(const std::size_t& clusteringId, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad) const
{
    bool ret = false;
    std::vector<int> idvec;
    getSimilarClustering(clusteringId, idvec);
    LOG(INFO)<<"clustering = "<<clusteringId<<", similar clustering size = "<<idvec.size();
    for (std::size_t i = 0; i < idvec.size(); ++i )
    {
        ret |= getAD(idvec[i], ad);
    }
    return ret;
}

void TopnClusteringModel::getSimilarClustering(
    const std::size_t& clusteringId, 
    std::vector<int>& similar) const
{
    similar = similarClustering_[clusteringId];
}

float TopnClusteringModel::score( 
    const std::string& text,
    const std::vector<std::pair<int, float> >& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    LaserOnlineModel onlineModel;
    if (!pUserDb_->get(text, onlineModel))
    {
        return ret;
    }
    // per-user
    static const std::vector<std::pair<int, float> > context; 
    ret += onlineModel.score(text, context, ad, (float)0.0);
    return ret;
}

float TopnClusteringModel::score( 
    const std::string& text,
    const std::vector<float>& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    LaserOnlineModel onlineModel;
    if (!pUserDb_->get(text, onlineModel))
    {
        return ret;
    }
    // per-user
    static const std::vector<std::pair<int, float> > context; 
    ret += onlineModel.score(text, context, ad, (float)0.0);
    return ret;
}
    
void TopnClusteringModel::dispatch(const std::string& method, msgpack::rpc::request& req)
{
    if ("updateTopNClustering" == method)
    {
        updateTopClusteringDb(req);
    }
    else if ("updatePerUserModel" == method)
    {
        updatepUserDb(req);
    }
    req.error("No handler for " + method);
}
    
void TopnClusteringModel::updatepUserDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, std::string, float, std::vector<float> > params;
    req.params().convert(&params);
    LaserOnlineModel onlineModel(params.get<2>(), params.get<3>());
    const std::string uuid(params.get<1>());
    bool res = pUserDb_->update(uuid, onlineModel);
    req.result(res);
}

void TopnClusteringModel::updateTopClusteringDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, std::string, std::map<int, float> > params;
    req.params().convert(&params);

    bool res = topClusteringDb_->update(params.get<1>(), params.get<2>());
    req.result(res);
}

} }
