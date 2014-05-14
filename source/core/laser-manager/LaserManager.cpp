#include "LaserManager.h"
#include "clusteringmanager/clustering/PCARunner.h"
#include <mining-manager/MiningManager.h>
#include <query-manager/ActionItem.h>
#include <ad-manager/AdSearchService.h>
#include <glog/logging.h>
using namespace sf1r::laser;
namespace sf1r
{
    
std::vector<boost::unordered_map<std::string, float> >* LaserManager::clusteringContainer_ = NULL;
laser::Tokenizer* LaserManager::tokenizer_ = NULL;
laser::LaserRpcServer* LaserManager::rpcServer_ = NULL;
laser::TopNClusteringDB* LaserManager::topnClustering_ = NULL;
laser::LaserOnlineModelDB* LaserManager::laserOnlineModel_ = NULL;
boost::shared_mutex LaserManager::mutex_;

LaserManager::LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService)
    : adSearchService_(adSearchService)
    , workdir_(MiningManager::system_working_path_ + "/LASER/")
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }

    load_();
    
    laser::AdIndexManager* index = new laser::AdIndexManager(workdir_);
    indexManager_.reset(index);
    recommend_.reset(new LaserRecommend(index, topnClustering_, laserOnlineModel_));
}
   
LaserManager::~LaserManager()
{
    close_();
}
    
void LaserManager::load_()
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    if (NULL != clusteringContainer_)
        return;
    clusteringContainer_ = new std::vector<TokenVector>();
    laser::clustering::loadClusteringResult(*clusteringContainer_, MiningManager::system_resource_path_ + "/laser/clustering_result");
    
    tokenizer_ = new Tokenizer(MiningManager::system_resource_path_ + "/dict/title_pca/",
        MiningManager::system_resource_path_ + "/laser/terms_dic.dat");

    topnClustering_ = new TopNClusteringDB(workdir_ + "/topnclustering");
    laserOnlineModel_ = new LaserOnlineModelDB(workdir_ + "/laser_online_model/"); 
    
    rpcServer_ = new LaserRpcServer(tokenizer_, clusteringContainer_, topnClustering_, laserOnlineModel_);
    rpcServer_->start("0.0.0.0", 28611, 2);
}
    
void LaserManager::close_()
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    if (NULL != clusteringContainer_)
        return;
    if (NULL != rpcServer_)
    {
        rpcServer_->stop();
        delete rpcServer_;
        rpcServer_ = NULL;
    }
    if (NULL != tokenizer_)
    {
        delete tokenizer_;
        tokenizer_ = NULL;
    }
    if (NULL != clusteringContainer_)
    {
        delete clusteringContainer_;
        clusteringContainer_ = NULL;
    }
    if (NULL != topnClustering_)
    {
        delete topnClustering_;
        topnClustering_ = NULL;
    }
    if (NULL != laserOnlineModel_)
    {
        delete laserOnlineModel_;
        laserOnlineModel_ = NULL;
    }
}

bool LaserManager::recommend(const LaserRecommendParam& param, 
    RawTextResultFromMIA& res) const 
{
    std::vector<std::string> docIdList;
    std::vector<float> itemScoreList;
    if (!recommend_->recommend(param.uuid_, docIdList, itemScoreList, param.topN_))
        return false;
    GetDocumentsByIdsActionItem actionItem;
    for (std::size_t i = 0; i < docIdList.size(); ++i)
    {
        actionItem.docIdList_.push_back(docIdList[i]);
    }
    adSearchService_->getDocumentsByIds(actionItem, res);
    return true;
}
    
void LaserManager::index(const std::string& docid, const std::string& title)
{
    boost::unordered_map<std::string, float> vec;
    tokenizer_->tokenize(title, vec);
    std::size_t clusteringId = assignClustering_(vec);
    std::vector<std::pair<int,float> >numericVec;
    tokenizer_->numeric(vec, numericVec);
    indexManager_->index(clusteringId, docid, numericVec); 
}
    
std::size_t LaserManager::assignClustering_(const TokenVector& v) const
{
    std::size_t size = clusteringContainer_->size();
    float maxSim = 0.0;
    std::size_t maxId = 0;
    for (std::size_t i = 0; i < size; ++i)
    {
        float sim = similarity_((*clusteringContainer_)[i], v);
        if (sim > maxSim)
        {
            maxSim = sim;
            maxId = i;
        }
    }
    return maxId;
}

float LaserManager::similarity_(const TokenVector& lv, const TokenVector& rv) const
{
    float sim = 0.0;
    TokenVector::const_iterator it = lv.begin(); 
    for (; it != lv.end(); ++it)
    {
        TokenVector::const_iterator thisIt = rv.find(it->first);
        if ( rv.end() != thisIt)
        {
            sim += it->second * thisIt->second;
        }
    }
}

}
