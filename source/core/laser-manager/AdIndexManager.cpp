#include "AdIndexManager.h"
#include <boost/filesystem.hpp>
#include <util/izene_serialization.h>
#include <fstream>
#include <glog/logging.h>

namespace sf1r { namespace laser {

AdIndexManager::AdIndexManager(const std::string& workdir, const std::string& collection)
    : workdir_(workdir + "/" + collection + "/ad-index-manager/")
    , containerPtr_(NULL)
    , lastDocId_(0)
{
    if (!boost::filesystem::exists(workdir_))
    {
        if (!boost::filesystem::exists(workdir + "/" + collection))
        {
            boost::filesystem::create_directory(workdir + "/" + collection);
        }
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
        izenelib::util::izene_deserialization<ADVector> izd((char*)val_p->data, val_p->size);
        izd.read_image(advec);
        containerPtr_->clean_data(val_p);
    }
    return true;
}

void AdIndexManager::open_()
{
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
} }
