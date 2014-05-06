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
#include <boost/filesystem.hpp>

using namespace boost;

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
    return false;
}

void ClusteringDataStorage::release()
{
    DBModelType<ClusteringInfo>::get()->release();
    DBModelType<ClusteringData>::get()->release();
}

void ClusteringDataStorage::reload(const std::string& clusteringPath)
{
    if (!filesystem::exists(clusteringPath + "/" + suffix_data) ||
        !filesystem::exists(clusteringPath + "/" + suffix_info))
    {
        return;
    }
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    release();
    filesystem::path dataFrom(clusteringPath + "/" +suffix_data);
    filesystem::path dataTo(dbpath_ + "/" + suffix_data + "/");
    
    filesystem::path infoFrom(clusteringPath + "/" +suffix_info);
    filesystem::path infoTo(dbpath_ + "/" + suffix_info + "/");
    filesystem::remove_all(dataTo);
    filesystem::remove_all(infoTo);
    filesystem::create_directory(dataTo);
    filesystem::create_directory(infoTo);

    for (filesystem::directory_iterator it(dataFrom); it != filesystem::directory_iterator(); ++it)
    {
        filesystem::path dataTo(dbpath_ + "/" + suffix_data + "/" + it->path().filename().string());
        filesystem::copy_file(it->path(), dataTo);
    }
    for (filesystem::directory_iterator it(infoFrom); it != filesystem::directory_iterator(); ++it)
    {
        filesystem::path infoTo(dbpath_ + "/" + suffix_info + "/" + it->path().filename().string());
        filesystem::copy_file(it->path(), infoTo);
    }
    if(!DBModelType<ClusteringInfo>::get()->init(suffix_info, dbpath_) ||
        DBModelType<ClusteringData>::get()->init(suffix_data,dbpath_))
    {
        LOG(ERROR)<<"reload clustering error";
    }
    /*set<std::string> newdataset;
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
    }*/
}
bool ClusteringDataStorage::save(ClusteringData& cd, ClusteringInfo& ci)
{
    if(cd.clusteringHash != ci.clusteringHash)
    {
        return false;
    }
    stringstream hashstr;
    hashstr<<cd.clusteringHash;
    
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
    if (!sharedLock)
    {
        return false;
    }
    return DBModelType<ClusteringInfo>::get()->update(hashstr.str(), ci) && 
                DBModelType<ClusteringData>::get()->update(hashstr.str(), cd);
}


bool ClusteringDataStorage::loadClusteringData(hash_t cat_id, clustering::type::ClusteringData& cd)
{
    stringstream hashstr;
    hashstr<<cat_id;
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
    if (!sharedLock)
    {
        return false;
    }
    return DBModelType<ClusteringData>::get()->get(hashstr.str(), cd);
}
bool ClusteringDataStorage::loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci)
{
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
    if (!sharedLock)
    {
        return false;
    }
    return DBModelType<ClusteringInfo>::get()->get(ci);
}
bool ClusteringDataStorage::loadClusteringInfo(hash_t cat_id, clustering::type::ClusteringInfo& ci)
{
    stringstream hashstr;
    hashstr<<cat_id;
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
    if (!sharedLock)
    {
        return false;
    }
    return DBModelType<ClusteringInfo>::get()->get(hashstr.str(), ci);
}

ClusteringDataStorage::~ClusteringDataStorage()
{
    DBModelType<ClusteringInfo>::get()->release();
    DBModelType<ClusteringData>::get()->release();
}

}

} /* namespace clustering */
}
}
