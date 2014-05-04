#include "LaserManager.h"
#include <mining-manager/MiningManager.h>
#include <query-manager/ActionItem.h>
#include <ad-manager/AdSearchService.h>
#include <glog/logging.h>
using namespace sf1r::laser;
namespace sf1r
{
    LaserManager::LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService)
        : adSearchService_(adSearchService)
    {
        rpcServer_ = RpcServer::getInstance();
        if (!boost::filesystem::exists(MiningManager::system_working_path_ + "/laser_clustering"))
        {
            boost::filesystem::create_directory(MiningManager::system_working_path_ + "/laser_clustering");
        }
        
        if (!boost::filesystem::exists(MiningManager::system_working_path_ + "/laser_leveldb"))
        {
            boost::filesystem::create_directory(MiningManager::system_working_path_ + "/laser_leveldb");
        }

        rpcServer_->start("0.0.0.0", 
            28611, 
            2,
            MiningManager::system_working_path_ + "/laser_clustering/",
            MiningManager::system_working_path_ + "/laser_leveldb",
            MiningManager::system_resource_path_ + "/dict/title_pca/");

        recommend_.reset(new LaserRecommend());
    }
   
    LaserManager::~LaserManager()
    {
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
}
