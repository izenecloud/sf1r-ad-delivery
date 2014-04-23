/*
 * DataAdpater.h
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#ifndef CLUSTERINGDATAADATER_H_
#define CLUSTERINGDATAADATER_H_
#include "type/ClusteringData.h"
#include "type/ClusteringInfo.h"
#include <list>
namespace clustering
{
namespace type
{
class ClusteringDataAdapter
{
public:
    ClusteringDataAdapter() {}
    virtual bool save(clustering::type::ClusteringData& cd, clustering::type::ClusteringInfo& ci)=0;
    virtual bool loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd)=0;
    virtual bool loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& cd)=0;
    virtual bool loadClusteringInfos(std::vector<clustering::type::ClusteringInfo>&) = 0;
    virtual ~ClusteringDataAdapter() {};
};
}
}
#endif /* DATAADPATER_H_ */
