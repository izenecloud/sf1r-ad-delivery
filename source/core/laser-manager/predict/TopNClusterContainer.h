#ifndef SF1R_LASER_TOPN_CLUSTER_CONTAINER_H
#define SF1R_LASER_TOPN_CLUSTER_CONTAINER_H

#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <util/singleton.h>
#include <am/leveldb/Table.h>
#include <vector>
#include <string>
#include "TopNCluster.h"
namespace sf1r
{
namespace laser
{
namespace predict
{
    class TopNClusterContainer
    {
        typedef izenelib::am::leveldb::Table<std::string, ClusterContainer> TopNclusterLeveldbType;
        //typedef boost::unordered_map<std::string, ClusterContainer> unordered_map;
    //private:
        //typedef unordered_map::const_iterator const_iterator;
    public:
        ~TopNClusterContainer();
        inline static TopNClusterContainer* get()
        {
            return izenelib::util::Singleton<TopNClusterContainer>::get();
        }
        bool init(std::string topNClusterPath);
        bool release();
        void output();

        TopNClusterContainer();
    public:
        bool update(const TopNCluster& cluster);
        bool get(const std::string& user, ClusterContainer& cluster) ;
    private:
        TopNclusterLeveldbType* topNclusterLeveldb_;
        std::string topNClusterPath_;
    };
}
}
}

#endif
