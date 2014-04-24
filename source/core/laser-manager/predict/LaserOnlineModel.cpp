#include "LaserOnlineModel.h"

namespace sf1r
{
namespace laser
{
namespace predict
{
    LaserOnlineModel::LaserOnlineModel()
    {
    }

    LaserOnlineModel::~LaserOnlineModel()
    {
        release();
    }

    bool LaserOnlineModel::init(std::string path)
    {
        dbpath_ = path;
        db_ = new PerUserOnlineModelDBType(dbpath_);
        if(db_->open())
        {
            return true;
        }
        else
        {
            release();
            return false;
        }
    }

    void LaserOnlineModel::release()
    {
        if (NULL != db_)
        {
            db_->close();
            delete db_;
            db_ = NULL;
        }
    }

    bool LaserOnlineModel::update(const PerUserOnlineModel& model)
    {
        if(model.getUserName().compare("")!=0)
        {
            return db_->insert(model.getUserName(), model.getArgs());
        }
        return false;
    }

    bool LaserOnlineModel::get(const std::string& user, PerUserOnlineModel& model)
    {
        bool res =  db_->get(user, model);
        if(res == true)
        {
            model.setUserName(user);
        }
        return res;
    }
}
}
}
