#include "LaserRecommend.h"
#include "predict/PredictTool.h"

#include <glog/logging.h>

namespace sf1r
{
namespace laser
{
bool LaserRecommend::recommend(const std::string& uuid, 
    std::vector<std::string>& itemList, 
    std::vector<float>& itemScoreList, 
    const std::size_t num) const
{
    LOG(INFO)<<"user request: uuid="<<uuid<<"\t topn="<<topn;
    ClusterContainer clusters;
    PerUserOnlineModel puseronlineModel;
    if (TopNClusterContainer::get()->get(uuid, clusters) && LaserOnlineModel::get()->get(uuid, puseronlineModel))
    {
        MinHeapDocRankType docrankHeap = getTopN(clusters, puseronlineModel, num);
        while (!docrankHeap.empty())
        {
           struct DocRank doc =  docrankHeap.top();
           itemList.push_back(doc.doc_id_);
           itemList.push_back(doc.rank_);
           docrankHeap.pop();
        }
        return true;
    }
    else
    {
        return false;
    }
}

}

}
