#ifndef SF1R_LASER_LASER_OFFLINE_MODEL_DB_H
#define SF1R_LASER_LASER_OFFLINE_MODEL_DB_H

#include <am/leveldb/Table.h>
#include <util/singleton.h>
#include <string>
namespace sf1r { namespace laser {
class LaserOfflineModelDB
{
    typedef izenelib::am::leveldb::Table<std::string, std::vector<float> > LaserOfflineModelDBType;
public:
    LaserOfflineModelDB(const std::string& dbpath);
    ~LaserOfflineModelDB();
public:        
    bool update(const std::string& user, const std::vector<float>& model);
    bool get(const std::string& user, std::vector<float>& model) const;
private:
    LaserOfflineModelDBType* db_;
    const std::string dbpath_;
};
} }
#endif
