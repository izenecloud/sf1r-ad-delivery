#include "SlimController.h"
#include "CollectionHandler.h"

namespace sf1r {

void SlimController::recommend() {
    collectionHandler_->slimRecommend(request(), response());
}

}
