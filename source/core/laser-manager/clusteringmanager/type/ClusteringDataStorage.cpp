/*
 * ClusteringDataStorage.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: alex
 */

#include "ClusteringDataStorage.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include <3rdparty/msgpack/msgpack/type/tuple.hpp>
#include <sstream>
#include <list>
#include <boost/filesystem.hpp>

using namespace boost;


namespace sf1r { namespace laser { namespace clustering { namespace type {


const char* ClusteringDataStorage::suffix_data="clusteringdata";
const char* ClusteringDataStorage::suffix_info="clusteringinfo";

ClusteringDataStorage::ClusteringDataStorage()
    : clusteringInfo_(NULL)
    , clusteringData_(NULL)
{

}

bool ClusteringDataStorage::init(const std::string& dbpath, bool clusteringService)
{
    if (NULL == clusteringInfo_)
    {
        clusteringInfo_ = new DBModelType<ClusteringInfo>();
    }
    if (NULL == clusteringData_)
    {
        clusteringData_ = new DBModelType<ClusteringData>();
    }
    if (clusteringService)
    {
        // this directory should contain only sub-directory;
        dbpath_ = dbpath + "/ClusteringDataStorage/";
        if (!filesystem::exists(dbpath_))
        {
            filesystem::create_directory(dbpath_);
        }
        filesystem::path path(dbpath_);
        filesystem::directory_iterator it(path);
        for (; it != filesystem::directory_iterator(); ++it)
        {
            if (filesystem::is_directory(it->path()))
            {
                std::string dir = it->path().string();
                LOG(INFO)<<"data storage path = "<<it->path().string();
                if (!filesystem::exists(dir + "/" + suffix_info))
                {
                    filesystem::create_directory(dir + "/" + suffix_info);
                }
                if (!filesystem::exists(dir + "/" + suffix_data))
                {
                    filesystem::create_directory(dir + "/" + suffix_data);
                }
                if (!clusteringInfo_->init(suffix_info, dir) ||
                    !clusteringData_->init(suffix_data, dir))
                {
                    release();
                    return false;
                }
                break;
            }
        }
    
        // empty
        if (it == filesystem::directory_iterator())
        {
            std::stringstream ss;
            ss<<dbpath_<<"/"<<rand();
            const std::string dir = ss.str();
            filesystem::create_directory(dir);
            if (!filesystem::exists(dir + "/" + suffix_info))
            {
                filesystem::create_directory(dir + "/" + suffix_info);
            }
            if (!filesystem::exists(dir + "/" + suffix_data))
            {
                filesystem::create_directory(dir + "/" + suffix_data);
            }
            if (!clusteringInfo_->init(suffix_info, dir) ||
                !clusteringData_->init(suffix_data, dir))
            {
                release();
                return false;
            }
        }

        return true;
    }
    else
    {
        dbpath_ = dbpath;
        if (!clusteringInfo_->init(suffix_info, dbpath_) ||
            !clusteringData_->init(suffix_data, dbpath_))
        {
            release();
            return false;
        }
        return true;
    }
}

void ClusteringDataStorage::release()
{
    if (NULL != clusteringInfo_)
    {
        clusteringInfo_->release();
        delete clusteringInfo_;
        clusteringInfo_ = NULL;
    }
    if (NULL != clusteringData_)
    {
        clusteringData_->release();
        delete clusteringData_;
        clusteringData_ = NULL;
    }
}

bool ClusteringDataStorage::reload(const std::string& clusteringPath)
{
    if (!filesystem::exists(clusteringPath + "/" + suffix_data) ||
        !filesystem::exists(clusteringPath + "/" + suffix_info))
    {
        return false;
    }
    
    std::string previous;
    filesystem::path path(dbpath_);
    filesystem::directory_iterator it(path);
    for (; it != filesystem::directory_iterator(); ++it)
    {
        if (filesystem::is_directory(it->path()))
        {
            previous = it->path().string();
        }
    }
    std::string dir = "";
    do
    {
        std::stringstream ss;
        ss<<dbpath_<<"/"<<rand();
        dir = ss.str();
    } 
    while (filesystem::exists(dir));
    
    filesystem::create_directory(dir);
    filesystem::create_directory(dir + "/" + suffix_info);
    filesystem::create_directory(dir + "/" + suffix_data);

    
    filesystem::path infoFrom(clusteringPath + "/" +suffix_info);
    filesystem::path dataFrom(clusteringPath + "/" +suffix_data);
    
    for (filesystem::directory_iterator it(dataFrom); it != filesystem::directory_iterator(); ++it)
    {
        filesystem::path dataTo(dir + "/" + suffix_data + "/" + it->path().filename().string());
        filesystem::copy_file(it->path(), dataTo);
    }
    for (filesystem::directory_iterator it(infoFrom); it != filesystem::directory_iterator(); ++it)
    {
        filesystem::path infoTo(dir + "/" + suffix_info + "/" + it->path().filename().string());
        filesystem::copy_file(it->path(), infoTo);
    }
    

    DBModelType<ClusteringInfo>* clusteringInfo = new DBModelType<ClusteringInfo>();
    DBModelType<ClusteringData>* clusteringData = new DBModelType<ClusteringData>();
    if (!clusteringInfo->init(suffix_info, dir) || 
        !clusteringData->init(suffix_data, dir))
    {
        LOG(ERROR)<<"reload clustering error";
        return false;
    }

    {
        LOG(INFO)<<"ClustringDataStorage switch to "<<dir;
        // lock and switch
        boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
        release();
        clusteringInfo_ = clusteringInfo;
        clusteringData_ = clusteringData;
    }

    filesystem::remove_all(previous);
    return true;
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
    return clusteringInfo_->update(hashstr.str(), ci) && 
                clusteringData_->update(hashstr.str(), cd);
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
    return clusteringData_->get(hashstr.str(), cd);
}
bool ClusteringDataStorage::loadClusteringInfos(vector<clustering::type::ClusteringInfo>& ci)
{
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
    if (!sharedLock)
    {
        return false;
    }
    return clusteringInfo_->get(ci);
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
    return clusteringInfo_->get(hashstr.str(), ci);
}

ClusteringDataStorage::~ClusteringDataStorage()
{
    release();
}

} } } }
