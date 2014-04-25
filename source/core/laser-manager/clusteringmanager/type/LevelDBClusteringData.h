/*
 * LevelDBClusteringData.h
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_LEVELDBCLUSTERINGDATA_H_
#define SF1R_LASER_LEVELDBCLUSTERINGDATA_H_

#include "ClusteringDataAdapter.h"
#include <3rdparty/am/leveldb/db.h>
#include <string>
#include <list>
#include <util/singleton.h>
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{


class LevelDBClusteringData: public ClusteringDataAdapter
{
private:
    leveldb::DB* db;
    std::string dbpath_;
    static const char* suffix_data;
    static const char* suffix_info;
public:
    LevelDBClusteringData();
    inline static LevelDBClusteringData* get()
    {
        return izenelib::util::Singleton<LevelDBClusteringData>::get();
    }
    bool init(std::string dbpath);
    void release();
    void reload()
    {
        //TODO for alex
    }
    bool save(clustering::type::ClusteringData& cd, clustering::type::ClusteringInfo& ci);
    bool loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd);
    bool loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& cd);
    bool loadClusteringData(hash_t cat_id, string&);
    bool loadClusteringInfo(hash_t cat_id, string&);
    bool loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci);
    bool loadLevelDBData(const string&, string&);
    inline string getKey(const std::string& suffix, hash_t cat_id)
    {
        stringstream cdkeystream;
        cdkeystream<<suffix<<cat_id;
        return cdkeystream.str();
    }
    virtual ~LevelDBClusteringData();
};
}
} /* namespace clustering */
}
}
#endif /* LEVELDBCLUSTERINGDATA_H_ */
