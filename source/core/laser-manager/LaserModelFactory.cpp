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
{
}

LaserModel* LaserModelFactory::createModel(const LaserPara& para,
    const std::string& workdir) const
{
    LOG(INFO)<<"Create Laser model for type = "<<para.modelType;
    if ("LaserGenericModel" == para.modelType)
    {
        return new LaserGenericModel(adIndexer_, para.kvaddr, para.kvport, para.mqaddr, para.mqport, workdir);
    }
    else if ("TopnClusteringModel" == para.modelType)
    {
        return new TopnClusteringModel(adIndexer_, similarClustering_, workdir);
    }
    else if ("HierarchicalModel" == para.modelType)
    {
        return new HierarchicalModel(adIndexer_, para.kvaddr, para.kvport, para.mqaddr, para.mqport, workdir);
    }
    LOG(ERROR)<<"No Laser model for type = "<<para.modelType<<", LaserRecommend stays unavailable.";
    return NULL;
}

} }
