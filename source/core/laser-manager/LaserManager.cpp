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
        rpcServer_.reset(new RpcServer(conf_.msgpack.host, 
            conf_.msgpack.port, conf_.msgpack.numThread));
        LOG(INFO)<<"start msgpack server on host:"<<
            conf_.msgpack.host<<" port: "<< conf_.msgpack.port;
        rpcServer_->init(conf_.clustering.leveldbRootPath,
            conf_.clustering.dbClusteringPath,
            conf_.clustering.dbPerUserModelPath);

        rpcServer_->start();

        recommend_.reset(new LaserRecommend());
    }
   
    LaserManager::~LaserManager()
    {
        LOG(INFO)<<"inform msgpack to exit";
        rpcServer_->stop();
        rpcServer_->join();
        LOG(INFO)<<"msgpack exited";
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
