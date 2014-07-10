#include "LaserModelFactory.h"
#include "LaserManager.h"
#include "TopnClusteringModel.h"
#include "LaserGenericModel.h"
#include "HierarchicalModel.h"
#include "AdIndexManager.h"

#include <glog/logging.h>

namespace sf1r { namespace laser {

LaserModelFactory::LaserModelFactory(const LaserManager& laserManager)
    : similarClustering_(*(laserManager.similarClustering_))
    , adIndexer_(*(laserManager.indexManager_))
    , sysdir_(laserManager.sysdir_)
{
}

LaserModel* LaserModelFactory::createModel(const LaserPara& para,
    const std::string& workdir) const
{
    LOG(INFO)<<"Create Laser model for type = "<<para.modelType;
    const docid_t adDimension = 2000000; //adIndexer_.getLastDocId();
    const std::size_t clusteringDimension = similarClustering_.size();
    if ("LaserGenericModel" == para.modelType)
    {
        return new LaserGenericModel(adIndexer_, para.kvaddr, para.kvport, para.mqaddr, para.mqport, workdir, sysdir_, adDimension);
    }
    else if ("TopnClusteringModel" == para.modelType)
    {
        return new TopnClusteringModel(adIndexer_, similarClustering_, workdir, sysdir_);
    }
    else if ("HierarchicalModel" == para.modelType)
    {
        return new HierarchicalModel(adIndexer_, para.kvaddr, para.kvport, para.mqaddr, para.mqport, workdir, sysdir_, adDimension, clusteringDimension);
    }
    LOG(ERROR)<<"No Laser model for type = "<<para.modelType<<", LaserRecommend stays unavailable.";
    return NULL;
}

} }
