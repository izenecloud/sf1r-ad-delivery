#ifndef SF1R_LASER_CAT_DICTIONARY_H
#define SF1R_LASER_CAT_DICTIONARY_H
#include <boost/unordered_map.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <util/singleton.h>
#include <fstream>
#include <glog/logging.h>

namespace sf1r { namespace laser { namespace clustering { namespace type {
/**
 * CatDictionary store the final category information,
 * It contains a file which record a list of catfile (catfile contains the clusteringresult in a certain orignal cat)
 *
 */
class CatDictionary
{
private:
    string dicPath_;
    boost::unordered_map<std::string, int> catDic_;
public:
    void close()
    {
        std::ofstream ofs(dicPath_.c_str(), std::ofstream::binary | std::ofstream::trunc);
        boost::archive::text_oarchive oa(ofs);
        try
        {
            oa << catDic_;
        }
        catch(std::exception& e)
        {
            LOG(INFO)<<e.what();
        }
    }

    inline static CatDictionary* get()
    {
        return izenelib::util::Singleton<CatDictionary>::get();
    }

    bool init(string dicpath, bool isLoad = false)
    {
        dicPath_ = (dicpath + "/category_dic.dat");
        if (!isLoad)
            return true;
        return load(dicPath_);
    }

    inline int get(const string& cat)
    {
        boost::unordered_map<std::string, int>::iterator iter = catDic_.find(cat);
        if (iter != catDic_.end())
        {
            return iter->second;
        }
        else
        {
            return -1;
        }
    }

    inline int add(const string& cat, int df = 1)
    {
        boost::unordered_map<std::string, int>::iterator iter = catDic_.find(cat);
        if (iter != catDic_.end())
        {
            iter->second += df;
            return iter->second;
        }
        else
        {
            catDic_[cat] = df;
            return df;
        }
    }

    bool load(const std::string& dicPath)
    {
        if (boost::filesystem::exists(dicPath))
        {
            std::ifstream ifs(dicPath.c_str(), std::ios::binary);
            boost::archive::text_iarchive ia(ifs);
            ia >> catDic_;
            return true;
        }
        return false;
    }

    boost::unordered_map<std::string, int>& getCatDic()
    {
        return catDic_;
    }

    string getDicPath() const
    {
        return dicPath_;
    }

    void setDicPath(string dicPath)
    {
        dicPath_ = dicPath;
    }
};

} } } }
#endif /* CATDICTIONARY_H_ */
