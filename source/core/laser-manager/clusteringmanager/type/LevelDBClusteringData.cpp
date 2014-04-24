/*
 * LevelDBClusteringData.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#include "LevelDBClusteringData.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include <3rdparty/msgpack/msgpack/type/tuple.hpp>
#include <list>
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{

const char* LevelDBClusteringData::suffix_data="clusteringdata_";
const char* LevelDBClusteringData::suffix_info="clusteringinfo_";
LevelDBClusteringData::LevelDBClusteringData():db(NULL),dbpath_("")
{

}

bool LevelDBClusteringData::init(std::string dbpath)
{
    dbpath_ = dbpath;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, dbpath_, &db);
    if(!status.ok())
    {
        LOG(INFO)<<status.ToString()<<endl;
        delete db;
        db = NULL;
        return false;
    }
    return true;
}

void LevelDBClusteringData::release()
{
    if(db != NULL)
    {
        delete db;
        db = NULL;
    }
}

bool LevelDBClusteringData::save(clustering::type::ClusteringData& cd, clustering::type::ClusteringInfo& ci)
{
    if(cd.clusteringHash != ci.clusteringHash)
    {
        return false;
    }
    if(db == NULL)
        return false;
    string cdstr;
    string cistr;
    serialization<clustering::type::ClusteringData>(cdstr, cd);
    serialization<clustering::type::ClusteringInfo>(cistr, ci);
    string cdkeystr = getKey(suffix_data, cd.clusteringHash);
    string cikeystr = getKey(suffix_info, ci.clusteringHash);
    leveldb::Slice cdkey(cdkeystr);
    leveldb::Slice cikey(cikeystr);
    leveldb::WriteOptions write_options;
    leveldb::Status cdstatus = db->Put(write_options, cdkey, cdstr);
    leveldb::Status cistatus = db->Put(write_options, cikey, cistr);
    if(!cdstatus.ok() || !cistatus.ok())
    {
        cout<<"check fail reason: clusteringdata:"<<cdstatus.ToString()<<"clusteringinfo"<<cistatus.ToString()<<endl;
        if(cdstatus.ok())
        {
            db->Delete(leveldb::WriteOptions(), cdkey);
        }
        if(cistatus.ok())
        {
            db->Delete(leveldb::WriteOptions(), cikey);
        }
        return false;
    }
    return true;
}


bool LevelDBClusteringData::loadClusteringData(hash_t cat_id, string& value)
{
    return loadLevelDBData(getKey(suffix_data,cat_id), value);
}
bool LevelDBClusteringData::loadClusteringInfo(hash_t cat_id, string& value)
{
    return loadLevelDBData(getKey(suffix_info,cat_id), value);
}
bool LevelDBClusteringData::loadLevelDBData(const string& key, string& value)
{
    if(db == NULL)
        return false;
    leveldb::Slice cdkey(key);
    leveldb::Status s = db->Get(leveldb::ReadOptions(), cdkey,  &value);
    return s.ok();
}
bool LevelDBClusteringData::loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd)
{
    std::string propertyValue;
    bool res = loadClusteringData(cat_id, propertyValue);
    if(res == false)
        return false;
    deserialize<clustering::type::ClusteringData>(propertyValue, cd);
    return true;
}
bool LevelDBClusteringData::loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci)
{
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    // cout<<"to read"<<endl;
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
        if(it->key().ToString().find(LevelDBClusteringData::suffix_info) == string::npos)
        {
            continue;
        }
        const string& value = it->value().ToString() ;
        ClusteringInfo newinfo;
        deserialize<ClusteringInfo>(value, newinfo);
	LOG(INFO)<<newinfo.clusteringHash<<endl;
        ci.push_back(newinfo);
    }
    ci.reserve(ci.size());
    delete it;
    return true;
}
bool LevelDBClusteringData::loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& ci)
{
    std::string propertyValue;
    bool res = loadClusteringInfo(cat_id, propertyValue);
    if(res == false)
        return false;
    deserialize<clustering::type::ClusteringInfo>(propertyValue, ci);
    return true;
}

LevelDBClusteringData::~LevelDBClusteringData()
{
    if(db != NULL)
    {
        delete db;
        db = NULL;
    }
}

}

} /* namespace clustering */
}
}
