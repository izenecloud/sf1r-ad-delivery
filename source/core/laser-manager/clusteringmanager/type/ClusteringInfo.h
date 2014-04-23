/*
 * ClusteringInfo.h
 *
 *  Created on: Apr 8, 2014
 *      Author: alex
 */

#ifndef CLUSTERINGINFO_H_
#define CLUSTERINGINFO_H_
#include<string>
#include<vector>
#include <3rdparty/msgpack/msgpack.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
using std::string;
namespace clustering
{
namespace type
{
struct ClusteringInfo
{
    string clusteringname;
    string clusteringMidPath;
    string clusteringPowPath;
    string clusteringResPath;
    map<hash_t, float> clusteringPow;
    int clusteringDocNum;
    hash_t clusteringHash;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & clusteringname;
        ar & clusteringDocNum;
        ar & clusteringHash;
        ar & clusteringPow;
    }
//    MSGPACK_DEFINE(clusteringname, clusteringDocNum, clusteringPow);
    MSGPACK_DEFINE(clusteringname, clusteringDocNum, clusteringHash, clusteringPow);
};
}
}


#endif /* CLUSTERINGINFO_H_ */
