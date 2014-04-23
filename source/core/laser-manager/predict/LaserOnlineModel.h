#ifndef LASER_LASER_ONLINE_MODEL_H
#define LASER_LASER_ONLINE_MODEL_H
#include <am/leveldb/Table.h>
#include <util/singleton.h>
#include <string>
#include "PerUserOnlineModel.h"
#include "TopNCluster.h"
namespace predict
{
    class LaserOnlineModel
    {
        typedef izenelib::am::leveldb::Table<std::string, PerUserOnlineModel> PerUserOnlineModelDBType;
    public:
        ~LaserOnlineModel();
        LaserOnlineModel();
        bool init(std::string path);
        inline static LaserOnlineModel* get()
        {
            return izenelib::util::Singleton<LaserOnlineModel>::get();
        }
        void release();
        bool update(const PerUserOnlineModel& model);
        bool get(const std::string& user, PerUserOnlineModel& model);
    private:
        PerUserOnlineModelDBType* db_;
        std::string dbpath_;
    };
}

#endif
