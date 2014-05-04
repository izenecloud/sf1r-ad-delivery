/*
 * ClusteringDataStorage.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#include "ClusteringDataStorage.h"
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

const char* ClusteringDataStorage::suffix_data="clusteringdata";
const char* ClusteringDataStorage::suffix_info="clusteringinfo";
ClusteringDataStorage::ClusteringDataStorage(): dbpath_("")
{

}

bool ClusteringDataStorage::init(std::string dbpath)
{
    dbpath_ = dbpath;
    if(DBModelType<ClusteringInfo>::get()->init(suffix_info, dbpath_) && 
        DBModelType<ClusteringData>::get()->init(suffix_data,dbpath_) )
    {
        return true;
    }
    //else
    //{
    //    release();
    //    return false;
    //}
}

void ClusteringDataStorage::release()
{
    DBModelType<ClusteringInfo>::get()->release();
    DBModelType<ClusteringData>::get()->release();
}

void ClusteringDataStorage::reload(const std::string& clusteringPath)
{
    set<std::string> newdataset;
    set<std::string> newinfoset;
    set<std::string> olddataset;
    set<std::string> oldinfoset;
    set<std::string> todeldataset;
    set<std::string> todelinfoset;
    std::string clusteringInfoPath = clusteringPath+"/infos";
    std::string clusteringDataPath = clusteringPath+"/data";
    izene_reader_pointer iDataFile = openFile<izene_reader>(clusteringDataPath, false);
    izene_reader_pointer iInfoFile = openFile<izene_reader>(clusteringInfoPath, false);
    if(iDataFile == NULL || iInfoFile == NULL)
        return;
    hash_t key;
    ClusteringData cd;
    ClusteringInfo ci;
    while(iDataFile->Next(key, cd)) {
        stringstream ss;
        ss<<key;
        newdataset.insert(ss.str());
        DBModelType<ClusteringData>::get()->update(key, cd);
    }
    while(iInfoFile->Next(key, ci)) {
        stringstream ss;
        ss<<key;
        newinfoset.insert(ss.str());
        DBModelType<ClusteringInfo>::get()->update(key, ci);
    }
    closeFile<izene_reader>(iDataFile);
    closeFile<izene_reader>(iInfoFile);
    DBModelType<ClusteringInfo>::get()->getKeys(oldinfoset);
    DBModelType<ClusteringData>::get()->getKeys(olddataset);
    for(set<std::string>::iterator iter = oldinfoset.begin(); iter != oldinfoset.end(); iter++)
    {
        if(newinfoset.find(*iter) == newinfoset.end())
        {
            todelinfoset.insert(*iter);
        }
    }
    for(set<std::string>::iterator iter = olddataset.begin(); iter != olddataset.end(); iter++)
    {
        if(newdataset.find(*iter) == newdataset.end())
        {
            todeldataset.insert(*iter);
        }
    }
    for(set<std::string>::iterator iter = todeldataset.begin(); iter != todeldataset.end(); iter++)
    {
        DBModelType<ClusteringData>::get()->del(*iter);
    }
    for(set<std::string>::iterator iter = todelinfoset.begin(); iter != todelinfoset.end(); iter++)
    {
        DBModelType<ClusteringInfo>::get()->del(*iter);
    }
}
bool ClusteringDataStorage::save(ClusteringData& cd, ClusteringInfo& ci)
{
    if(cd.clusteringHash != ci.clusteringHash)
    {
        return false;
    }
    stringstream hashstr;
    hashstr<<cd.clusteringHash;
    return DBModelType<ClusteringInfo>::get()->update(hashstr.str(), ci) && 
                DBModelType<ClusteringData>::get()->update(hashstr.str(), cd);
//    DBModelType<ClusteringInfo>::get()->release();
//    DBModelType<ClusteringData>::get()->release();

    /*    if(db == NULL)
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
        return true;*/
}


bool ClusteringDataStorage::loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd)
{
    stringstream hashstr;
    hashstr<<cat_id;
    return DBModelType<ClusteringData>::get()->get(hashstr.str(), cd);
}
bool ClusteringDataStorage::loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci)
{
    return DBModelType<ClusteringInfo>::get()->get(ci);
    /*    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
        // cout<<"to read"<<endl;
        for (it->SeekToFirst(); it->Valid(); it->Next())
        {
            if(it->key().ToString().find(ClusteringDataStorage::suffix_info) == string::npos)
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
        return true;*/
}
bool ClusteringDataStorage::loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& ci)
{
    stringstream hashstr;
    hashstr<<cat_id;
    return DBModelType<ClusteringInfo>::get()->get(hashstr.str(), ci);

    /*    std::string propertyValue;
        bool res = loadClusteringInfo(cat_id, propertyValue);
        if(res == false)
            return false;
        deserialize<clustering::type::ClusteringInfo>(propertyValue, ci);
        return true;*/
}

ClusteringDataStorage::~ClusteringDataStorage()
{
    DBModelType<ClusteringInfo>::get()->release();
    DBModelType<ClusteringData>::get()->release();
    /*    if(db != NULL)
        {
            delete db;
            db = NULL;
        }*/
}

}

} /* namespace clustering */
}
}
