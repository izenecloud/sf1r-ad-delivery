/*
 * ClusteringInfo.h
 *
 *  Created on: Apr 8, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_CLUSTERING_INFO_H
#define SF1R_LASER_CLUSTERING_INFO_H
#include <string>
#include <vector>
#include <3rdparty/msgpack/msgpack.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/unordered_map.hpp>

#include "laser-manager/clusteringmanager/common/constant.h"

namespace sf1r { namespace laser { namespace clustering { namespace type {
class ClusteringInfo
{
public:
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & clusteringname;
        ar & clusteringDocNum;
        ar & clusteringHash;
        ar & clusteringPow;
    }
    MSGPACK_DEFINE(clusteringname, clusteringDocNum, clusteringHash, clusteringPow);
public:
    const boost::unordered_map<std::string, float>& getClusteringVector() const
    {
        return clusteringPow;
    }
public:
    std::string clusteringname;
    std::string clusteringMidPath;
    std::string clusteringPowPath;
    std::string clusteringResPath;
    boost::unordered_map<std::string, float> clusteringPow;
    int clusteringDocNum;
    hash_t clusteringHash;
};
} } } }

#endif /* CLUSTERINGINFO_H_ */
