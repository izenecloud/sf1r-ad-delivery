#include "LaserController.h"
#include "CollectionHandler.h"
#include <laser-manager/clusteringmanager/type/LevelDBClusteringData.h>

namespace sf1r
{
void LaserController::recommend()
{
    collectionHandler_->laserRecomend(request(), response());
}

void LaserController::load_clustering()
{
    laser::clustering::type::LevelDBClusteringData::get()->reload();
}
}
