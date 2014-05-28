#include "AdIndexManager.h"
#include <boost/filesystem.hpp>
#include <util/izene_serialization.h>
#include <fstream>
#include <glog/logging.h>

namespace sf1r { namespace laser {

AdIndexManager::AdIndexManager(const std::string& workdir, 
        const boost::shared_ptr<DocumentManager>& documentManager)
    : workdir_(workdir + "/index/")
    , containerPtr_(NULL)
    , lastDocId_(0)
    , documentManager_(documentManager)
//    , cache_(NULL)
{
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    containerPtr_ = new ContainerType(Lux::IO::NONCLUSTER);
    containerPtr_->set_noncluster_params(Lux::IO::Linked);
    containerPtr_->set_lock_type(Lux::IO::LOCK_THREAD);
    open_();
}

AdIndexManager::~AdIndexManager()
{
    if (NULL != containerPtr_)
    {
        containerPtr_->close();
        delete containerPtr_;
    }
    serializeLastDocid_();
}

void AdIndexManager::index(const docid_t& docid, 
    const std::vector<std::pair<int, float> >& vec)
{
    izenelib::util::izene_serialization<std::vector<std::pair<int, float> > > izs(vec);
    char* src;
    size_t srcLen;
    izs.write_image(src, srcLen);
    containerPtr_->put(docid, src, srcLen, Lux::IO::OVERWRITE);
}

bool AdIndexManager::get(const docid_t& docid, 
    std::vector<std::pair<int, float> >& vec) const
{
    {
        return false;
    }

    Lux::IO::data_t *val_p = NULL;
    if (!containerPtr_->get(docid, &val_p, Lux::IO::SYSTEM))
    {
        containerPtr_->clean_data(val_p);
        return false;
    }
    else if (documentManager_->isDeleted(docid))
    {
        return false;
    }
    else
    {
        izenelib::util::izene_deserialization<std::vector<std::pair<int, float> > > izd((char*)val_p->data, val_p->size);
        izd.read_image(vec);
        containerPtr_->clean_data(val_p);
    }
    return true;
}
    
void AdIndexManager::index(const std::size_t& clusteringId, 
        const docid_t& docid, 
        const std::vector<std::pair<int, float> >& vec)
{
    ADVector advec;
    get(clusteringId, advec);

    AD ad;
    ad.first = docid;
    ad.second = vec;
    advec.push_back(ad);
    izenelib::util::izene_serialization<ADVector> izs(advec);
    char* src;
    size_t srcLen;
    izs.write_image(src, srcLen);
    containerPtr_->put(clusteringId, src, srcLen, Lux::IO::OVERWRITE);
}

bool AdIndexManager::get(const std::size_t& clusteringId, ADVector& advec) const
{
    Lux::IO::data_t *val_p = NULL;
    if (!containerPtr_->get(clusteringId, &val_p, Lux::IO::SYSTEM))
    {
        containerPtr_->clean_data(val_p);
        return false;
    }
    else
    {
        ADVector vec;
        izenelib::util::izene_deserialization<ADVector> izd((char*)val_p->data, val_p->size);
        izd.read_image(vec);
        containerPtr_->clean_data(val_p);
        ADVector::const_iterator it = vec.begin();
        for (; it != vec.end(); ++it)
        {
            if (!documentManager_->isDeleted(it->first))
            {
                advec.push_back(*it);
            }
        }
    }
    return true;
}

void AdIndexManager::open_()
{
    LOG(INFO)<<"AdIndexManager worddir = "<<workdir_;
    try
    {
        const std::string filename = workdir_ + "/index";
        if ( !boost::filesystem::exists(filename) )
        {
            containerPtr_->open(filename.c_str(), Lux::IO::DB_CREAT);
        }
        else
        {
            containerPtr_->open(filename.c_str(), Lux::IO::DB_RDWR);
        }
    }
    catch (...)
    {
    }

   {
        const std::string filename = workdir_ + "/last_docid";
        if ( boost::filesystem::exists(filename) )
        {
            std::ifstream ifs(filename.c_str(), std::ios::binary);
            boost::archive::text_iarchive ia(ifs);
            ia >> lastDocId_;
        }
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

} }
