#include "TopNController.h"
#include "LaserOnlineModel.h"
#include "PerUserOnlineModel.h"
#include <util/driver/Request.h>
#include <string>
#include "Request.h"
#include "Response.h"
#include "Compaign.h"
#include "LaserOnlineModel.h"
#include "TopNClusterContainer.h"
#include "PerUserOnlineModel.h"
#include "PredictTool.h"
#include <glog/logging.h>
namespace predict
{
    void TopNController::top()
    {
        Request req;
        req.parseRequest(request());
        const std::string& uuid = req.getUuid();
        const int topn = req.getTopN();
        LOG(INFO)<<"user request: uuid="<<uuid<<"\t topn="<<topn;
        ClusterContainer clusters;
        PerUserOnlineModel puseronlineModel;
        if (TopNClusterContainer::get()->get(uuid, clusters) && LaserOnlineModel::get()->get(uuid, puseronlineModel))
        {
            MinHeapDocRankType docrankHeap = getTopN(clusters, puseronlineModel, topn);
        }
        else
        {
            // TODO no result
        }
        {
            // TODO
            Response res;
            for (int i = 0; i < topn; i++)
            {
                res.push_back(new Compaign());
            }
            res.render(response());
        }
    }
}
