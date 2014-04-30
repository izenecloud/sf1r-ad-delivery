/*
 * ClusteringDataStorage.h
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_CLUSTERINGDATASTORAGE_H_
#define SF1R_LASER_CLUSTERINGDATASTORAGE_H_

#include "ClusteringDataAdapter.h"
#include <3rdparty/am/leveldb/db.h>
#include <string>
#include <list>
#include <util/singleton.h>
#include "ModelType.h"
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{


class ClusteringDataStorage: public ClusteringDataAdapter
{
private:
    //leveldb::DB* db;
    std::string dbpath_;
    static const char* suffix_data;
    static const char* suffix_info;
public:
    ClusteringDataStorage();
    inline static ClusteringDataStorage* get()
    {
        return izenelib::util::Singleton<ClusteringDataStorage>::get();
    }
    bool init(std::string dbpath);
    void release();
    void reload(const std::string& clusteringPath);
    bool save(ClusteringData& cd, ClusteringInfo& ci);
    bool loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd);
    bool loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& cd);
    bool loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci);
    virtual ~ClusteringDataStorage();
};
}
} /* namespace clustering */
}
}
#endif /* SF1R_LASER_CLUSTERINGDATASTORAGE_H_ */
