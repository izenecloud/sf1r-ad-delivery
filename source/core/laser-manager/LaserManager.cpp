#include "LaserManager.h"
#include <glog/logging.h>
using namespace sf1r::laser;
namespace sf1r
{
    LaserManager::LaserManager(const boost::shared_ptr<AdSearchService>& adSearchService,
        const LaserConfig& laserConfig)
        : adSearchService_(adSearchService)
        , conf_(laserConfig)
    {
        rpcServer_ = RpcServer::getInstance();
        rpcServer_->start(conf_.msgpack.host, 
            conf_.msgpack.port, 
            conf_.msgpack.numThread,
            conf_.clustering.resultRoot,
            conf_.clustering.leveldbRootPath,
            conf_.clustering.dictionarypath);

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
