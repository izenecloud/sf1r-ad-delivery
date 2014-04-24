/*
 * DataAdpater.h
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_CLUSTERINGDATAADATER_H_
#define SF1R_LASER_CLUSTERINGDATAADATER_H_
#include "ClusteringData.h"
#include "ClusteringInfo.h"
#include <list>
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{
class ClusteringDataAdapter
{
public:
    ClusteringDataAdapter() {}
    virtual bool save(ClusteringData& cd, ClusteringInfo& ci)=0;
    virtual bool loadClusteringData(hash_t cat_id, ClusteringData& cd)=0;
    virtual bool loadClusteringInfo(hash_t cat_id, ClusteringInfo& cd)=0;
    virtual bool loadClusteringInfos(std::vector<ClusteringInfo>&) = 0;
    virtual ~ClusteringDataAdapter() {};
};
}
}
}
}
#endif /* DATAADPATER_H_ */
