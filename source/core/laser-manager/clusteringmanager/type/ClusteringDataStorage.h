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

namespace sf1r { namespace laser { namespace clustering { namespace type {


class ClusteringDataStorage: public ClusteringDataAdapter
{
public:
    ClusteringDataStorage();
    inline static ClusteringDataStorage* get()
    {
        return izenelib::util::Singleton<ClusteringDataStorage>::get();
    }
    bool init(const std::string& dbpath);
    void release();
    bool reload(const std::string& clusteringPath);
    bool save(ClusteringData& cd, ClusteringInfo& ci);
    bool loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd);
    bool loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& cd);
    bool loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci);
    virtual ~ClusteringDataStorage();
private:
    boost::shared_mutex mutex_;
    std::string dbpath_;
    
    DBModelType<ClusteringInfo>* clusteringInfo_;
    DBModelType<ClusteringData>* clusteringData_;
    
    static const char* suffix_data;
    static const char* suffix_info;
};
} } } }
#endif /* SF1R_LASER_CLUSTERINGDATASTORAGE_H_ */
