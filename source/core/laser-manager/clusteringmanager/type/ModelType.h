#ifndef SF1R_LASER_DB_MODEL_H
#define SF1R_LASER_DB_MODEL_H
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <util/singleton.h>
#include <string>
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{
template <class Model>
class DBModelType
{
    typedef izenelib::am::leveldb::Table<std::string, Model> ModelDBTableType;
public:
    virtual ~DBModelType()
    {
        release();
    }
    DBModelType()
    {

    }
    bool init(std::string suffix, std::string path)
    {
        dbpath_ = path+"/"+suffix;
        db_ = new ModelDBTableType(dbpath_);
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
    inline static DBModelType<Model>* get()
    {
        return izenelib::util::Singleton<DBModelType<Model> >::get();
    }
    void release()
    {
        if (NULL != db_)
        {
            db_->close();
            delete db_;
            db_ = NULL;
        }
    }
    bool del(const std::string& key)
    {
        return db_->del(key);
    }
    bool update(const std::string& key, const Model& model)
    {
        return db_->insert(key, model);
    }
    bool update(const hash_t& key, const Model& model)
    {
        std::stringstream ss;
        ss<<key;
        return db_->insert(ss.str(), model);
    }
    bool get(const std::string& key, Model& model)
    {
        return db_->get(key, model);
    }
    bool get(vector<Model>& v)
    {
        typedef izenelib::am::AMIterator<ModelDBTableType> AMIteratorType;
        AMIteratorType iter(*db_);
        AMIteratorType end;
        for(; iter!= end; ++iter)
        {
            v.push_back(iter->second);
        }
        return true;
    }
    bool getKeys(set<string>& s)
    {
        typedef izenelib::am::AMIterator<ModelDBTableType> AMIteratorType;
        AMIteratorType iter(*db_);
        AMIteratorType end;
        for(; iter!= end; ++iter)
        {
            s.insert(iter->first);
        }
        return true;
    }
private:
    ModelDBTableType* db_;
    std::string dbpath_;
};
}
}
}
}
#endif
