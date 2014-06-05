#include "LaserModelFactory.h"
#include "LaserManager.h"
#include "TopnClusteringModel.h"
#include "LaserGenericModel.h"
#include "HierarchicalModel.h"
#include "AdIndexManager.h"

namespace sf1r { namespace laser {

LaserModelFactory::LaserModelFactory(const LaserManager& laserManager)
    : similarClustering_(*(laserManager.similarClustering_))
    , adIndexer_(*(laserManager.indexManager_))
{
}

LaserModel* LaserModelFactory::createModel(const LaserConfig& config,
    const std::string& workdir,
    const std::size_t ncandidate) const
{
    if (!config.isEnableClustering())
    {
        return new LaserGenericModel(adIndexer_, workdir);
    }
    else if (config.isEnableTopnClustering)
    {
        return new TopnClusteringModel(adIndexer_, similarClustering_, workdir, ncandidate);
    }
    else if (config.isEnableHierarchical)
    {
        return new HierarchicalModel(adIndexer_, workdir, ncandidate);
    }
    return NULL;
}

} }
