#include "LaserManager.h"
#include <mining-manager/MiningManager.h>
#include <glog/logging.h>
using namespace sf1r::laser;
namespace sf1r
{
    LaserManager::LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService)
        : adSearchService_(adSearchService)
    {
        rpcServer_ = RpcServer::getInstance();
        rpcServer_->start("0.0.0.0", 
            28611, 
            2,
            MiningManager::system_resource_path_ + "/laser_clustering/",
            MiningManager::system_working_path_ + "/laser_leveldb",
            MiningManager::system_resource_path_ + "/dict/title_pca/");

        recommend_.reset(new LaserRecommend());
    }
   
    LaserManager::~LaserManager()
    {
    }
    
    bool LaserManager::recommend(const LaserRecommendParam& param, 
        RawTextResultFromMIA& itemList) const 
    {
        std::vector<std::string> docIdList;
        std::vector<float> itemScoreList;
        if (!recommend_->recommend(param.uuid_, docIdList, itemScoreList, param.topN_))
            return false;
        //TODO
        //miningManager_.GetAdSearchService()->getDocument(docIdList, itemList);
        return true;
    }
}
