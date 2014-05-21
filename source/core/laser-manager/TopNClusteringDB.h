#ifndef SF1R_LASER_TOPN_CLUSTERING_DB_H
#define SF1R_LASER_TOPN_CLUSTERING_DB_H

#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <util/singleton.h>
#include <am/leveldb/Table.h>
#include <vector>
#include <string>
#include "TopNClustering.h"
namespace sf1r { namespace laser {

class TopNClusteringDB
{
    typedef izenelib::am::leveldb::Table<std::string, std::map<int, float> > TopNclusterLeveldbType;
public:
    TopNClusteringDB(const std::string& topNClusteringPath,
        const std::size_t& maxClustering);
    ~TopNClusteringDB();

public:
    bool update(const std::string& user, const std::map<int, float>& clustering);
    bool get(const std::string& user, std::map<int, float>& clustering) const;

private:
    bool load_();

private:
    TopNclusterLeveldbType* topNclusterLeveldb_;
    std::string topNClusteringPath_;
    const std::size_t maxClustering_;
    const static std::size_t TOP_N = 10;
};
} }

#endif
