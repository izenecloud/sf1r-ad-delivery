#include "TopnClusteringModel.h"
#include "TopNClusteringDB.h"
#include "LaserOnlineModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include "SparseVector.h"

namespace sf1r { namespace laser {
TopnClusteringModel::TopnClusteringModel(const AdIndexManager& adIndexer,
    const std::vector<std::vector<int> >& similarClustering,
    const std::string& workdir,
    const std::string& sysdir)
    : adIndexer_(adIndexer)
    , similarClustering_(similarClustering)
    , workdir_(workdir)
    , sysdir_(sysdir)
    , pUserDb_(NULL)
    , origUserDb_(NULL)
    , topClusteringDb_(NULL)
    , origClusteringDb_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    const std::string origUserDB = sysdir_ + "/orig-per-user-online-model";
    origUserDb_ = new LaserModelDB<std::string, LaserOnlineModel>(origUserDB);
    const std::string userDB = workdir_ + "/per-user-online-model";
    bool loadUserDB = false;
    if (!boost::filesystem::exists(userDB))
    {
        loadUserDB = true;
    }
    pUserDb_ = new LaserModelDB<std::string, LaserOnlineModel>(userDB);
    
    const std::string origCDB = sysdir_ + "/orig-per-user-topn-clustering";
    origClusteringDb_= new LaserModelDB<std::string, std::vector<std::pair<int, float> > >(origCDB);
    const std::string cDB = workdir_ + "/per-user-topn-clustering";
    bool loadCDB = false;
    if (!boost::filesystem::exists(cDB))
    {
        loadCDB = true;
    }
    topClusteringDb_ = new LaserModelDB<std::string, std::vector<std::pair<int, float> > >(cDB);
    localizeFromOrigDB(loadUserDB, loadCDB);
}

TopnClusteringModel::~TopnClusteringModel()
{
    if (NULL != pUserDb_)
    {
        delete pUserDb_;
        pUserDb_ = NULL;
    }
    if (NULL != origUserDb_)
    {
        delete origUserDb_;
        origUserDb_ = NULL;
    }
    if (NULL != topClusteringDb_)
    {
        delete topClusteringDb_;
        topClusteringDb_ = NULL;
    }
    if (NULL != origClusteringDb_)
    {
        delete origClusteringDb_;
        origClusteringDb_ = NULL;
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

    std::vector<std::pair<int, float> > topn;
    if (!topClusteringDb_->get(text, topn))
    {
        return false;
    }
    std::vector<std::pair<int, float> >::const_iterator it = topn.begin();
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

    std::vector<std::pair<int, float> > topn;
    if (!topClusteringDb_->get(text, topn))
    {
        return false;
    }
    std::vector<std::pair<int, float> >::const_iterator it = topn.begin();
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
    if ("update_topn_clustering" == method)
    {
        updateTopClusteringDb(req);
    }
    else if ("finish_offline_model" == method)
    {
        req.result(true);
    }
    else if ("update_online_model" == method)
    {
        updatepUserDb(req);
    }
    else if ("finish_online_model" == method)
    {
        req.result(true);
    }
    else
    {
        req.error("No handler for " + method);
    }
}
    
void TopnClusteringModel::updatepUserDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, LaserOnlineModel> params;
    req.params().convert(&params);
    const std::string uuid(params.get<0>());
    bool res = pUserDb_->update(uuid, params.get<1>());
    req.result(res);
}

void TopnClusteringModel::updateTopClusteringDb(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string, SparseVector> params;
    req.params().convert(&params);
    const SparseVector& sv = params.get<1>();
    const std::vector<int>& index = sv.index();
    const std::vector<float>& value = sv.value();
    std::vector<std::pair<int, float> > topn(index.size());
    for (std::size_t i = 0; i < index.size(); ++i)
    {
        topn[i] = std::make_pair(index[i], value[i]);
    }
    bool res = topClusteringDb_->update(params.get<0>(), topn);
    req.result(res);
}
    
void TopnClusteringModel::localizeFromOrigDB(bool loadUDB, bool loadCDB)
{
    if (loadUDB)
    {
        LOG(INFO)<<"localize per-user-online-model from original model, since localized model doesn't exist. \
            This procedure may be slow, be patient";
        LaserModelDB<std::string, LaserOnlineModel>::iterator it = origUserDb_->begin();
        for (; it != origUserDb_->end(); ++it)
        {
            pUserDb_->update(it->first, it->second);
        }
    }
    if (loadCDB)
    {
        LOG(INFO)<<"localize per-user-topn-clustering from original model, since localized model doesn't exist. \
            This procedure may be slow, be patient";
        LaserModelDB<std::string, std::vector<std::pair<int, float> > >::iterator it = origClusteringDb_->begin();
        for (; it != origClusteringDb_->end(); ++it)
        {
             topClusteringDb_->update(it->first, it->second);
        }
    }
}

} }
