#include "AdIndexManager.h"
#include <boost/filesystem.hpp>
#include <util/izene_serialization.h>
#include <fstream>
#include <glog/logging.h>

namespace sf1r { namespace laser {

AdIndexManager::AdIndexManager(const std::string& workdir, 
        const bool isEnableClustering, 
        const boost::shared_ptr<DocumentManager>& documentManager)
    : workdir_(workdir + "/index/")
    , isEnableClustering_(isEnableClustering)
    , adPtr_(NULL)
    , clusteringPtr_(NULL)
    , lastDocId_(0)
    , documentManager_(documentManager)
//    , cache_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    open_();
    deserializeLastocid_();
}

AdIndexManager::~AdIndexManager()
{
    close_();
    serializeLastDocid_();
}

void AdIndexManager::index(const docid_t& docid, 
    const std::vector<std::pair<int, float> >& vec)
{
    izenelib::util::izene_serialization<std::vector<std::pair<int, float> > > izs(vec);
    char* src;
    size_t srcLen;
    izs.write_image(src, srcLen);
    adPtr_->put(docid, src, srcLen, Lux::IO::OVERWRITE);
}

bool AdIndexManager::get(const std::size_t& clusteringId, ADVector& advec) const
{
    std::vector<docid_t> docidList;
    if (!get(clusteringId, docidList))
    {
        return false;
    }
    for (std::size_t i = 0; i < docidList.size(); ++i)
    {
        std::vector<std::pair<int, float> > vec;
        if (get(docidList[i], vec))
        {
            advec.push_back(std::make_pair(docidList[i], vec));
        }
    }
    return true;
}

bool AdIndexManager::get(const docid_t& docid, std::size_t& clustering) const
{
    std::pair<std::size_t, std::vector<std::pair<int, float> > > docinfo;
    bool ret = get(docid, docinfo);
    if (ret)
    {
        clustering = docinfo.first;
    }
    return ret;
}

bool AdIndexManager::get(const docid_t& docid, 
    std::vector<std::pair<int, float> > & vec) const
{
    std::pair<std::size_t, std::vector<std::pair<int, float> > > docinfo;
    bool ret = get(docid, docinfo);
    if (ret)
    {
        vec = docinfo.second;
    }
    return ret;
}

bool AdIndexManager::get(const docid_t& docid, 
    std::pair<std::size_t, std::vector<std::pair<int, float> > >& vec) const
{
    Lux::IO::data_t *val_p = NULL;
    if (!adPtr_->get(docid, &val_p, Lux::IO::SYSTEM))
    {
        adPtr_->clean_data(val_p);
        return false;
    }
    else if (documentManager_->isDeleted(docid))
    {
        return false;
    }
    else
    {
        izenelib::util::izene_deserialization<std::pair<std::size_t, std::vector<std::pair<int, float> > > > izd((char*)val_p->data, val_p->size);
        izd.read_image(vec);
        adPtr_->clean_data(val_p);
    }
    return true;
}

bool AdIndexManager::get(const std::size_t& clusteringId, std::vector<docid_t>& docids) const
{
    Lux::IO::data_t *val_p = NULL;
    if (!clusteringPtr_->get(clusteringId, &val_p, Lux::IO::SYSTEM))
    {
        clusteringPtr_->clean_data(val_p);
        return false;
    }
    else
    {
        izenelib::util::izene_deserialization<std::vector<docid_t> > izd((char*)val_p->data, val_p->size);
        izd.read_image(docids);
        clusteringPtr_->clean_data(val_p);
    }
    return true;
}

void AdIndexManager::index(const std::size_t& clusteringId, 
        const docid_t& docid,
        const std::vector<std::pair<int, float> >& vec)
{
    {
        std::vector<docid_t> docidList;
        get(clusteringId, docidList);

        docidList.push_back(docid);
        izenelib::util::izene_serialization<std::vector<docid_t> > izs(docidList);
        char* src;
        size_t srcLen;
        izs.write_image(src, srcLen);
        clusteringPtr_->put(clusteringId, src, srcLen, Lux::IO::OVERWRITE);
    }
    {
        izenelib::util::izene_serialization<std::pair<std::size_t, std::vector<std::pair<int, float> > > > izs(std::make_pair(docid, vec));
        char* src;
        size_t srcLen;
        izs.write_image(src, srcLen);
        adPtr_->put(docid, src, srcLen, Lux::IO::OVERWRITE);
    }
}

void AdIndexManager::open_()
{
    try
    {
        LOG(INFO)<<"open ad-index...";
        adPtr_ = new ContainerType(Lux::IO::NONCLUSTER);
        adPtr_->set_noncluster_params(Lux::IO::Linked);
        adPtr_->set_lock_type(Lux::IO::LOCK_THREAD);
        const std::string filename = workdir_ + "/ad-index";
        if (!boost::filesystem::exists(filename) )
        {
            adPtr_->open(filename.c_str(), Lux::IO::DB_CREAT);
        }
        else
        {
            adPtr_->open(filename.c_str(), Lux::IO::DB_RDWR);
        }
        if (isEnableClustering_)
        {
            LOG(INFO)<<"open clustering-index";
            clusteringPtr_= new ContainerType(Lux::IO::NONCLUSTER);
            clusteringPtr_->set_noncluster_params(Lux::IO::Linked);
            clusteringPtr_->set_lock_type(Lux::IO::LOCK_THREAD);
            const std::string filename = workdir_ + "/clustering-index";
            if (!boost::filesystem::exists(filename) )
            {
                clusteringPtr_->open(filename.c_str(), Lux::IO::DB_CREAT);
            }
            else
            {
                clusteringPtr_->open(filename.c_str(), Lux::IO::DB_RDWR);
            }
        }
    }
    catch (...)
    {
    }
}

void AdIndexManager::close_()
{
    if (NULL != adPtr_)
    {
        LOG(INFO)<<"close ad-index";
        adPtr_->close();
        delete adPtr_;
        adPtr_ = NULL;
    }
    if (NULL != clusteringPtr_)
    {
        LOG(INFO)<<"close clustering-index";
        clusteringPtr_->close();
        delete clusteringPtr_;
        clusteringPtr_ = NULL;
    }
}
    
void AdIndexManager::preIndex()
{
}

void AdIndexManager::postIndex()
{
    serializeLastDocid_();
}
    
void AdIndexManager::serializeLastDocid_()
{
    const std::string filename = workdir_ + "/last_docid";
    std::ofstream ofs(filename .c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << lastDocId_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}
    
void AdIndexManager::deserializeLastocid_()
{
    const std::string filename = workdir_ + "/last_docid";
    if ( boost::filesystem::exists(filename) )
    {
        std::ifstream ifs(filename.c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> lastDocId_;
    }
}

} }
