/*
 * PredictTool.h
 *
 *  Created on: Apr 16, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_PREDICT_PREDICTTOOL_H
#define SF1R_LASER_PREDICT_PREDICTTOOL_H_

#include <queue>
#include <string>
#include "TopNClusterContainer.h"
#include "PerUserOnlineModel.h"
#include "laser-manager/clusteringmanager/type/ClusteringInfo.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataStorage.h"
#include "laser-manager/clusteringmanager/type/Document.h"
namespace sf1r
{
namespace laser
{
namespace predict
{

    struct DocRank{
        string doc_id_;
        float rank_;
        DocRank(string docid, float rank)
        {
            doc_id_ = docid;
            rank_ = rank;
        }
    };

    struct comparator{
        bool operator() ( DocRank i, DocRank j){
            return i.rank_ > j.rank_;
       }
    };

    typedef std::priority_queue<DocRank, std::vector<DocRank>, comparator> MinHeapDocRankType;
    void getTopN(const ClusterContainer& clusters,const PerUserOnlineModel& puseronlineModel, size_t topN, MinHeapDocRankType& minHeap);
}
}
}
#endif /* PREDICTTOOL_H_ */
