#include "TopNClusteringDB.h"
#include <am/range/AmIterator.h>
#include <boost/filesystem.hpp>

using namespace izenelib::am;
namespace sf1r { namespace laser {

TopNClusteringDB::TopNClusteringDB(const std::string& topNClusteringPath)
    : topNClusteringPath_(topNClusteringPath)
{
    if (!boost::filesystem::exists(topNClusteringPath_))
    {
        boost::filesystem::create_directory(topNClusteringPath_);
    }
    load_();
}

TopNClusteringDB::~TopNClusteringDB()
{
    if (NULL != topNclusterLeveldb_)
    {
        topNclusterLeveldb_->close();
        delete topNclusterLeveldb_;
        topNclusterLeveldb_ = NULL;
    }
}

bool TopNClusteringDB::load_()
{
    topNclusterLeveldb_ = new TopNclusterLeveldbType(topNClusteringPath_);
    if(topNclusterLeveldb_->open())
    {
        return true;
    }
    else
    {
        topNclusterLeveldb_->close();
        delete topNclusterLeveldb_;
        topNclusterLeveldb_ = NULL;
        return false;
    }
}

bool TopNClusteringDB::update(const std::string& user, const std::map<int, float>& clustering)
{
    return topNclusterLeveldb_->insert(user, clustering);
}

bool TopNClusteringDB::get(const std::string& user, std::map<int, float>& clustering) const
{
    return topNclusterLeveldb_->get(user, clustering);
}
} }
