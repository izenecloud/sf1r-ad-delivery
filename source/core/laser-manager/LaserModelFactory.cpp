#include "LaserModelFactory.h"
#include "LaserManager.h"
#include "TopnClusteringModel.h"
#include "LaserGenericModel.h"
#include "HierarchicalModel.h"
#include "AdIndexManager.h"
#include "LSHIndexModel.h"

#include <glog/logging.h>

namespace sf1r { namespace laser {

LaserModelFactory::LaserModelFactory(const LaserManager& laserManager)
    : similarClustering_(laserManager.similarClustering_)
    , adIndexer_(*(laserManager.indexManager_))
    , sysdir_(laserManager.sysdir_)
{
}

LaserModel* LaserModelFactory::createModel(const LaserPara& para,
    const std::string& workdir) const
{
    LOG(INFO)<<"Create Laser model for type = "<<para.modelType;
    const docid_t adDimension = adIndexer_.getLastDocId();
    if ("LaserGenericModel" == para.modelType)
    {
        return new LaserGenericModel(adIndexer_, para.kvaddr, 
            para.kvport, para.mqaddr, para.mqport, workdir, sysdir_, adDimension, para.AD_FD, para.USER_FD);
    }
    else if ("TopnClusteringModel" == para.modelType)
    {
        return new TopnClusteringModel(adIndexer_, *similarClustering_, workdir, sysdir_);
    }
    else if ("HierarchicalModel" == para.modelType)
    {
        const std::size_t clusteringDimension = similarClustering_->size();
        return new HierarchicalModel(adIndexer_, para.kvaddr, 
            para.kvport, para.mqaddr, para.mqport, workdir, sysdir_, adDimension, clusteringDimension, para.AD_FD, para.USER_FD);
    }
    else if ("LSHIndexModel" == para.modelType)
    {
        return new LSHIndexModel(adIndexer_, para.kvaddr, para.kvport, 
            para.mqaddr, para.mqport, workdir, sysdir_, adDimension, para.AD_FD, para.USER_FD);
    }
    LOG(ERROR)<<"No Laser model for type = "<<para.modelType<<", LaserRecommend stays unavailable.";
    return NULL;
}

} }
