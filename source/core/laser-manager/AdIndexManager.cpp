#include "AdIndexManager.h"
#include "LaserManager.h"

#include <boost/filesystem.hpp>
#include <util/izene_serialization.h>
#include <fstream>

#include <mining-manager/MiningManager.h>
#include <glog/logging.h>

namespace sf1r { namespace laser {

AdIndexManager::AdIndexManager(const std::string& workdir, 
        const bool isEnableClustering, 
        LaserManager* laserManager)
    : workdir_(workdir + "/index/")
    , isEnableClustering_(isEnableClustering)
    , adPtr_(NULL)
    , adClusteringPtr_(NULL)
    , clusteringPtr_(NULL)
    , lastDocId_(0)
    , laserManager_(laserManager)
    , documentManager_(laserManager_->documentManager_)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directories(workdir_);
    }
    adPtr_ = new std::vector<std::vector<std::pair<int, float> > >();
    loadAdIndex();
    if (isEnableClustering_)
    {
        clusteringPtr_ = new std::vector<std::vector<docid_t> >(laserManager_->clusteringContainer_->size());
        adClusteringPtr_ = new std::vector<std::size_t>();
        loadClusteringIndex();
    }
}

AdIndexManager::~AdIndexManager()
{
    if (NULL != clusteringPtr_)
    {
        delete clusteringPtr_;
        clusteringPtr_ = NULL;
    }
    if (NULL != adClusteringPtr_)
    {
        delete adClusteringPtr_;
        adClusteringPtr_ = NULL;
    }
    if (NULL != adPtr_)
    {
        delete adPtr_;
        adPtr_ = NULL;
    }
}

void AdIndexManager::index(const docid_t& docid, 
    const std::vector<std::pair<int, float> >& vec)
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mtx_);
    if (adPtr_->size() <= docid)
    {
        adPtr_->resize(docid + 1);
    }
    (*adPtr_)[docid] = vec;
}

void AdIndexManager::index(const std::size_t& clusteringId, 
        const docid_t& docid,
        const std::vector<std::pair<int, float> >& vec)
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mtx_);
    (*clusteringPtr_)[clusteringId].push_back(docid);
    if (adClusteringPtr_->size() <= docid)
    {
        adClusteringPtr_->resize(docid + 1);
    }
    (*adClusteringPtr_)[docid] = clusteringId;
    if (adPtr_->size() <= docid)
    {
        adPtr_->resize(docid + 1);
    }
    (*adPtr_)[docid] = vec;
}

bool AdIndexManager::get(const std::size_t& clusteringId, ADVector& advec) const
{
    //boost::shared_lock<boost::shared_mutex> sharedLock(mtx_);
    if (clusteringId >= clusteringPtr_->size())
    {
        return false;
    }
    const std::vector<docid_t>& docidList = (*clusteringPtr_)[clusteringId];
    for (std::size_t i = 0; i < docidList.size(); ++i)
    {
        advec.push_back(std::make_pair(docidList[i], (*adPtr_)[docidList[i]]));
    }
    return true;
}

bool AdIndexManager::get(const docid_t& docid, std::size_t& clustering) const
{
    if (docid >= adClusteringPtr_->size())
    {
        return false;
    }
    clustering = (*adClusteringPtr_)[docid];
    return true;
}

bool AdIndexManager::get(const docid_t& docid, 
    std::vector<std::pair<int, float> > & vec) const
{
    if (docid >= adPtr_->size())
    {
        return false;
    }
    vec = (*adPtr_)[docid];
    return true;
}

bool AdIndexManager::get(const std::size_t& clusteringId, std::vector<docid_t>& docids) const
{
    if (clusteringId >= clusteringPtr_->size())
    {
        return false;
    }
    docids = (*clusteringPtr_)[clusteringId];
    return true;
}


void AdIndexManager::preIndex()
{
    laserManager_->recommend_->preUpdateAdDimension();
}

void AdIndexManager::postIndex()
{
    saveAdIndex();
    if (isEnableClustering_)
    {
        saveClusteringIndex();
    }
    laserManager_->recommend_->updateAdDimension(lastDocId_);
    laserManager_->recommend_->postUpdateAdDimension();
}
    
void AdIndexManager::saveAdIndex()
{
    LOG(INFO)<<"save ad-index...";
    const std::string filename = workdir_ + "ad-index";
    std::ofstream ofs(filename.c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << *adPtr_;
        oa << lastDocId_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}
    
void AdIndexManager::loadAdIndex()
{
    const std::string filename = workdir_ + "ad-index";
    if (!boost::filesystem::exists(filename))
    {
        return;
    }
    LOG(INFO)<<"open ad-index...";
    try
    {
        std::ifstream ifs(filename.c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> *adPtr_;
        ia >> lastDocId_;
    }
    catch (std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
}

void AdIndexManager::saveClusteringIndex()
{
    LOG(INFO)<<"save clustering-index...";
    const std::string filename = workdir_ + "clustering-index";
    std::ofstream ofs(filename.c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << *clusteringPtr_;
        oa << *adClusteringPtr_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}
    
void AdIndexManager::loadClusteringIndex()
{
    const std::string filename = workdir_ + "clustering-index";
    if (!boost::filesystem::exists(filename))
    {
        return;
    }
    LOG(INFO)<<"open clustering-index...";
    try
    {
        std::ifstream ifs(filename.c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> *clusteringPtr_;
        ia >> *adClusteringPtr_;
    }
    catch (std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
}
    
bool AdIndexManager::convertDocId(const std::string& docStr, docid_t& docId) const
{
    return laserManager_->convertDocId(docStr, docId);
}
} }
