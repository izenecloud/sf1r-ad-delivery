#include "LaserManager.h"
#include "clusteringmanager/clustering/PCARunner.h"
#include "LaserIndexTask.h"
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

LaserManager::LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService,
    const std::string& collection)
    : workdir_(MiningManager::system_working_path_ + "/LASER/")
    , collection_(collection)
    , adSearchService_(adSearchService)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }

    load_();
    
    laser::AdIndexManager* index = new laser::AdIndexManager(workdir_, collection_, clusteringContainer_->size());
    indexManager_.reset(index);
    recommend_.reset(new LaserRecommend(index, topnClustering_, laserOnlineModel_));
    indexTask_.reset(new LaserIndexTask(this));
}
   
LaserManager::~LaserManager()
{
    close_();
}
    
void LaserManager::load_()
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    if (NULL != clusteringContainer_ ||
        NULL != rpcServer_ ||
        NULL != tokenizer_ ||
        NULL != topnClustering_ ||
        NULL != laserOnlineModel_)
    {
        return;
    }
    LOG(INFO)<<"open all dependencies of LaserManager...";
    clusteringContainer_ = new std::vector<TokenVector>();
    laser::clustering::loadClusteringResult(*clusteringContainer_, MiningManager::system_resource_path_ + "/laser_resource/clustering_result");
    
    tokenizer_ = new Tokenizer(MiningManager::system_resource_path_ + "/dict/title_pca/",
        MiningManager::system_resource_path_ + "/laser_resource/terms_dic.dat");

    topnClustering_ = new TopNClusteringDB(workdir_ + "/topnclustering");
    laserOnlineModel_ = new LaserOnlineModelDB(workdir_ + "/laser_online_model/"); 
    
    rpcServer_ = new LaserRpcServer(tokenizer_, clusteringContainer_, topnClustering_, laserOnlineModel_);
    rpcServer_->start("0.0.0.0", 28611, 2);
}
    
void LaserManager::close_()
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    if (NULL == clusteringContainer_ ||
        NULL == rpcServer_ ||
        NULL == tokenizer_ ||
        NULL == topnClustering_ ||
        NULL == laserOnlineModel_)
    {
        return;
    }
    LOG(INFO)<<"close all dependencies of LaserManager..";
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
    std::vector<docid_t> docIdList;
    std::vector<float> itemScoreList;
    if (!recommend_->recommend(param.uuid_, docIdList, itemScoreList, param.topN_))
        return false;
    GetDocumentsByIdsActionItem actionItem;
    for (std::size_t i = 0; i < docIdList.size(); ++i)
    {
        actionItem.idList_.push_back(docIdList[i]);
    }
    adSearchService_->getDocumentsByIds(actionItem, res);
    return true;
}
    
void LaserManager::index(const docid_t& docid, const std::string& title)
{
    boost::unordered_map<std::string, float> vec;
    tokenizer_->tokenize(title, vec);
    std::size_t clusteringId = assignClustering_(vec);
    //LOG(INFO)<<docid<<"\t"<<title<<"\t clustering id = "<<clusteringId;
    std::vector<std::pair<int,float> >numericVec;
    tokenizer_->numeric(vec, numericVec);
    if ( -1 == clusteringId)
    {
        // for void
        clusteringId = rand() % clusteringContainer_->size();
    }
    indexManager_->index(clusteringId, docid, numericVec); 
}
    
MiningTask* LaserManager::getLaserIndexTask()
{
    return indexTask_.get();
}
    
const std::size_t LaserManager::getClustering(const std::string& title) const
{
    boost::unordered_map<std::string, float> vec;
    tokenizer_->tokenize(title, vec);
    return assignClustering_(vec);
}

std::size_t LaserManager::assignClustering_(const TokenVector& v) const
{
    std::size_t size = clusteringContainer_->size();
    float maxSim = 0.0;
    std::size_t maxId = -1;
    for (std::size_t i = 0; i < size; ++i)
    {
        float sim = similarity_(v, (*clusteringContainer_)[i]);
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
    return sim;
}

}
