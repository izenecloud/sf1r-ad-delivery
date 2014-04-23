/*
 * PredictTool.h
 *
 *  Created on: Apr 16, 2014
 *      Author: alex
 */

#ifndef PREDICTTOOL_H_
#define PREDICTTOOL_H_

#include <queue>
#include <string>
#include "TopNClusterContainer.h"
#include "PerUserOnlineModel.h"
#include "type/ClusteringInfo.h"
#include "type/LevelDBClusteringData.h"
#include "type/Document.h"
using std::queue;
using std::string;
using std::priority_queue;
using namespace clustering::type;
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

    typedef priority_queue<DocRank, std::vector<DocRank>, comparator> MinHeapDocRankType;
    MinHeapDocRankType getTopN(const ClusterContainer& clusters,const PerUserOnlineModel& puseronlineModel, size_t topN);
} /* namespace clustering */
#endif /* PREDICTTOOL_H_ */
