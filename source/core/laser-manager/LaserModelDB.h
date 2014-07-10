#ifndef SF1R_LASER_LASER_MODEL_DB_H
#define SF1R_LASER_LASER_MODEL_DB_H
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <util/singleton.h>
#include <string>
namespace sf1r { namespace laser {

template <typename K, typename V>
class LaserModelDB
{
public:
    typedef izenelib::am::leveldb::Table<K, V> LaserModelTable;
    typedef typename izenelib::am::AMIterator<LaserModelTable> iterator;
public:
    LaserModelDB(const std::string& dbpath)
        : dbpath_(dbpath)
    {
        if (!boost::filesystem::exists(dbpath_))
        {
            boost::filesystem::create_directory(dbpath_);
        }
        
        db_ = new LaserModelTable(dbpath_);
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
    ~LaserModelDB()
    {
        db_->close();
        delete db_;
        db_ = NULL;
    }
public:        
    bool update(const K& key, const V& value)
    {
        return db_->insert(key, value);
    }
    
    bool get(const K& key, V& value) const
    {
        return db_->get(key, value);
    }

    iterator begin() const
    {
        return iterator(*db_);
    }

    iterator end() const
    {
        return iterator();
    }
private:
    LaserModelTable* db_;
    const std::string dbpath_;
};

} }
#endif
