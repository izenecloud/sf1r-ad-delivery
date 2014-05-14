#include "LaserOnlineModelDB.h"
#include <boost/filesystem.hpp>

namespace sf1r { namespace laser {

    LaserOnlineModelDB::LaserOnlineModelDB(const std::string& dbpath)
        : dbpath_(dbpath)
    {
        if (!boost::filesystem::exists(dbpath_))
        {
            boost::filesystem::create_directory(dbpath_);
        }
        
        db_ = new LaserOnlineModelDBType(dbpath_);
        if(db_->open())
        {
        }
        else
        {
            db_->close();
            delete db_;
            db_ = NULL;
        }
    }

LaserOnlineModelDB::~LaserOnlineModelDB()
{
    db_->close();
    delete db_;
    db_ = NULL;
}

bool LaserOnlineModelDB::update(const std::string& user, const std::vector<float>& model)
{
    return db_->insert(user, model);
}

bool LaserOnlineModelDB::get(const std::string& user, std::vector<float>& model) const
{
    return db_->get(user, model);
}
} }
