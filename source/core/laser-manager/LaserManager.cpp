#include "LaserManager.h"
#include "clusteringmanager/clustering/PCARunner.h"
#include "LaserIndexTask.h"
#include "LaserRpcServer.h"
#include <mining-manager/MiningManager.h>
#include <query-manager/ActionItem.h>
#include <ad-manager/AdSearchService.h>
#include <glog/logging.h>
#include <sys/time.h>

using namespace sf1r::laser;
namespace sf1r
{

const static std::size_t THREAD_NUM = 8;
    
std::string LaserManager::workdir_ = "";
std::vector<LaserManager::TokenVector>* LaserManager::clusteringContainer_ = NULL;
std::vector<std::vector<int> >* LaserManager::similarClustering_ = NULL;
laser::Tokenizer* LaserManager::tokenizer_ = NULL;
laser::LaserRpcServer* LaserManager::rpcServer_ = NULL;
laser::TopNClusteringDB* LaserManager::topnClustering_ = NULL;
laser::LaserOnlineModelDB* LaserManager::laserOnlineModel_ = NULL;
boost::shared_mutex LaserManager::mutex_;

LaserManager::LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService, 
        const boost::shared_ptr<DocumentManager>& documentManager,
        const std::string& collection)
    : collection_(collection)
    , adSearchService_(adSearchService)
    , documentManager_(documentManager)
    , recommend_(NULL)
    , indexManager_(NULL)
{
    loadLaserDependency(); 
    indexManager_ = new laser::AdIndexManager(MiningManager::system_working_path_ + "/collection/" + collection_ + "/collection-data/", 
        clusteringContainer_->size(), 
        documentManager_);
    recommend_ = new LaserRecommend(indexManager_, topnClustering_, laserOnlineModel_, similarClustering_);
    // delete by TaskBuilder
    indexTask_ = new LaserIndexTask(this);

}
   
LaserManager::~LaserManager()
{
    if (NULL != indexManager_)
    {
        delete indexManager_;
        indexManager_ = NULL;
    }
    if (NULL != recommend_)
    {
        delete recommend_;
        recommend_ =NULL;
    }
    //delete indexTask_;
}
    
bool LaserManager::recommend(const LaserRecommendParam& param, 
    GetDocumentsByIdsActionItem& actionItem,
    RawTextResultFromMIA& res) const 
{
    std::vector<docid_t> docIdList;
    std::vector<float> itemScoreList;
    if (!recommend_->recommend(param.uuid_, docIdList, itemScoreList, param.topN_))
    {
        res.error_ = "Internal ERROR in Recommend Engine, no data for uuid: " + param.uuid_;;
        return false;
    }
    for (std::size_t i = 0; i < docIdList.size(); ++i)
    {
        actionItem.idList_.push_back(docIdList[i]);
    }
    adSearchService_->getDocumentsByIds(actionItem, res);
    return true;
}
    
void LaserManager::index(const docid_t& docid, const std::string& title)
{
    //struct timeval stime, etime;
    //gettimeofday(&stime, NULL);
    TokenSparseVector vec;
    tokenizer_->tokenize(title, vec);
    //gettimeofday(&etime, NULL);
    //LOG(INFO)<<"TOKENIZE : = "<<1000* (etime.tv_sec - stime.tv_sec) + (etime.tv_usec - stime.tv_usec);
    //gettimeofday(&stime, NULL);
    std::size_t clusteringId = assignClustering_(vec);
    //gettimeofday(&etime, NULL);
    //LOG(INFO)<<"ASSIGN : = "<<1000000* (etime.tv_sec - stime.tv_sec) + (etime.tv_usec - stime.tv_usec) <<"\t"<<clusteringId;
    if ( (std::size_t)-1 == clusteringId)
    {
        // for void
        clusteringId = rand() % clusteringContainer_->size();
    }
    //gettimeofday(&stime, NULL);
    indexManager_->index(clusteringId, docid, vec); 
    //gettimeofday(&etime, NULL);
    //LOG(INFO)<<"INDEX : = "<<1000000* (etime.tv_sec - stime.tv_sec) + (etime.tv_usec - stime.tv_usec);
}
    
MiningTask* LaserManager::getLaserIndexTask()
{
    return indexTask_;
}
    
const std::size_t LaserManager::getClustering(const std::string& title) const
{
    TokenSparseVector vec;
    tokenizer_->tokenize(title, vec);
    return assignClustering_(vec);
}

std::size_t LaserManager::assignClustering_(const TokenSparseVector& v) const
{
    if (v.empty())
    {
        return -1;
    }
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
    
float LaserManager::similarity_(const TokenSparseVector& lv, const TokenVector& rv) const
{
    float sim = 0.0;
    TokenSparseVector::const_iterator it = lv.begin(); 
    for (; it != lv.end(); ++it)
    {
        sim += rv[it->first] * it->second;
    }
    return sim;
}

    
void loadLaserDependency()
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(LaserManager::mutex_);
    if (NULL != LaserManager::clusteringContainer_ ||
        NULL != LaserManager::rpcServer_ ||
        NULL != LaserManager::tokenizer_ ||
        NULL != LaserManager::topnClustering_ ||
        NULL != LaserManager::laserOnlineModel_)
    {
        return;
    }
    LOG(INFO)<<"open all dependencies of LaserManager...";
    LaserManager::workdir_ = MiningManager::system_working_path_ + "/LASER/";
    if (!boost::filesystem::exists(LaserManager::workdir_))
    {
        boost::filesystem::create_directory(LaserManager::workdir_);
    }

    LaserManager::tokenizer_ = new Tokenizer(MiningManager::system_resource_path_ + "/dict/title_pca/",
        MiningManager::system_resource_path_ + "/laser_resource/terms_dic.dat");

    std::vector<boost::unordered_map<std::string, float> > clusteringContainer;
    laser::clustering::loadClusteringResult(clusteringContainer, MiningManager::system_resource_path_ + "/laser_resource/clustering_result");
    
    LaserManager::clusteringContainer_ = new std::vector<LaserManager::TokenVector>(clusteringContainer.size());
    for (std::size_t i = 0; i < clusteringContainer.size(); ++i)
    {
        LaserManager::TokenVector vec;
        LaserManager::tokenizer_->numeric(clusteringContainer[i], vec);
        (*LaserManager::clusteringContainer_)[i] = vec;
    }
    //LOG(INFO)<<clusteringContainer_->size(); 
   
    LaserManager::similarClustering_ = new std::vector<std::vector<int> >(LaserManager::clusteringContainer_->size());
    {
        std::ifstream ifs((MiningManager::system_resource_path_ + "/laser_resource/clustering_similar").c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> *LaserManager::similarClustering_;
    }

    LaserManager::topnClustering_ = new TopNClusteringDB(LaserManager::workdir_ + "/topnclustering", 
        LaserManager::clusteringContainer_->size());
    LaserManager::laserOnlineModel_ = new LaserOnlineModelDB(LaserManager::workdir_ + "/laser_online_model/"); 
    
    LaserManager::rpcServer_ = new LaserRpcServer(LaserManager::tokenizer_, 
        LaserManager::clusteringContainer_, 
        LaserManager::topnClustering_, 
        LaserManager::laserOnlineModel_);
    LaserManager::rpcServer_->start("0.0.0.0", 28611, 2);
}

void closeLaserDependency()
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(LaserManager::mutex_);
    if (NULL == LaserManager::clusteringContainer_ ||
        NULL == LaserManager::rpcServer_ ||
        NULL == LaserManager::tokenizer_ ||
        NULL == LaserManager::topnClustering_ ||
        NULL == LaserManager::laserOnlineModel_)
    {
        return;
    }
    LOG(INFO)<<"close all dependencies of LaserManager..";
    if (NULL != LaserManager::rpcServer_)
    {
        LaserManager::rpcServer_->stop();
        delete LaserManager::rpcServer_;
        LaserManager::rpcServer_ = NULL;
    }
    if (NULL != LaserManager::tokenizer_)
    {
        delete LaserManager::tokenizer_;
        LaserManager::tokenizer_ = NULL;
    }
    if (NULL != LaserManager::clusteringContainer_)
    {
        delete LaserManager::clusteringContainer_;
        LaserManager::clusteringContainer_ = NULL;
    }
    if (NULL != LaserManager::similarClustering_)
    {
        delete LaserManager::similarClustering_;
        LaserManager::similarClustering_ = NULL;
    }
    if (NULL != LaserManager::topnClustering_)
    {
        delete LaserManager::topnClustering_;
        LaserManager::topnClustering_ = NULL;
    }
    if (NULL != LaserManager::laserOnlineModel_)
    {
        delete LaserManager::laserOnlineModel_;
        LaserManager::laserOnlineModel_ = NULL;
    }
}

}