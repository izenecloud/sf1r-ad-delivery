#include "LaserModelDB.h"
#include <boost/filesystem.hpp>

namespace sf1r { namespace laser {

    LaserModelDB::LaserModelDB(const std::string& dbpath)
        : dbpath_(dbpath)
    {
        if (!boost::filesystem::exists(dbpath_))
        {
            boost::filesystem::create_directory(dbpath_);
        }
        
        db_ = new LaserModelDBType(dbpath_);
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

LaserModelDB::~LaserModelDB()
{
    db_->close();
    delete db_;
    db_ = NULL;
}

bool LaserModelDB::update(const std::string& user, const std::vector<float>& model)
{
    return db_->insert(user, model);
}

bool LaserModelDB::get(const std::string& user, std::vector<float>& model) const
{
    //for (int i = 0; i <= 1000; i++)
    //{
    //    model.push_back((rand() % 1000) / 1000.0);
    //}
    //return true;
    return db_->get(user, model);
}
} }
