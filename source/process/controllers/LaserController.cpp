#include "LaserController.h"
#include "Request.h"
#include "Response.h"

#include <glog/logging.h>
#include <util/driver/Request.h>
#include <common/ResultType.h>
#include <string>
namespace sf1r
{
void LaserController::recommend()
{
    collectionHandler_->laserRecomend(request(), response());
    
    Request req;
    req.parseRequest(request());
    const std::string& uuid = req.getUuid();
    const int topn = req.getTopN();
    LOG(INFO)<<"user request: uuid="<<uuid<<"\t topn="<<topn;
    
    std::vector<RawTextResultFromSIA> itemList;
    std::vector<float> itemScoreList;
}

}
