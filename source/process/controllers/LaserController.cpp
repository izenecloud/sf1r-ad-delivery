#include "LaserController.h"
#include "CollectionHandler.h"

namespace sf1r
{
void LaserController::recommend()
{
    collectionHandler_->laserRecomend(request(), response());
}

}
