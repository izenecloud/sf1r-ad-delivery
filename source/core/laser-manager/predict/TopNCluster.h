#ifndef LASER_TOPN_CLUSTER_H
#define LASER_TOPN_CLUSTER_H
#include <string>
#include <vector>
#include <map>
#include <boost/serialization/access.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include "clusteringmanager/common/constant.h"
namespace predict
{
    typedef std::map<clustering::hash_t, float>  ClusterContainer;
    typedef ClusterContainer::const_iterator ClusterContainerIter;
    class TopNCluster
    {
    public:
        void setUserName(std::string un)
        {
            user_ = un;
        }

        void setClusterContainer(const ClusterContainer& clusters)
        {
            clusters_ = clusters;
        }
        const std::string& getUserName() const
        {
            return user_;
        }

        const ClusterContainer& getTopNCluster() const
        {
            return clusters_;
        }

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
        //    ar & user_;
            ar & clusters_;
        }

        MSGPACK_DEFINE(user_, clusters_);
    private:
        std::string user_;
        ClusterContainer clusters_;
    };
}

#endif
