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
    if (boost::filesystem::exists(workdir_ + "per-clustering-online-model"))
    {
        load();
    }
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
    std::vector<std::pair<std::size_t, float> > clustering;
    for (std::size_t i = 0; i < clusteringDimension_; ++i)
    {
        static const std::pair<docid_t, std::vector<std::pair<int, float> > > perAd;
        clustering.push_back(std::make_pair(i, (*pClusteringDb_)[i].score(text, context, perAd, (float)0.0)));
    }
    
    typedef izenelib::util::second_greater<std::pair<std::size_t, float> > greater_than;
    std::sort(clustering.begin(), clustering.end());
    for (std::size_t i = 0; i < clustering.size(); ++i)
    {
        adIndexer_.get(clustering[i].first, ad);
        /*{
            // TODO remove just for test
            // per clustering 1k ad
            for (std::size_t c = 0; c < 1024; ++c)
            {
                docid_t adid = rand() % 2000000; // 2M ad
                std::vector<std::pair<int, float> > vec;
                for (int k = 0; k < 10; ++k)
                {
                    vec.push_back(std::make_pair(rand() % 10000, (rand() % 100 )/ 100.0));
                }
                ad.push_back(std::make_pair(adid, vec));
            }
        }*/
        if (ad.size() >= ncandidate)
            break;
    }
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
