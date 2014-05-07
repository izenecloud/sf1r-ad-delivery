#ifndef SF1R_LASER_CAT_DICTIONARY_H
#define SF1R_LASER_CAT_DICTIONARY_H
#include <boost/unordered_map.hpp>
#include <iostream>
#include <util/singleton.h>
#include <fstream>

namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{
/**
 * CatDictionary store the final category information,
 * It contains a file which record a list of catfile (catfile contains the clusteringresult in a certain orignal cat)
 *
 */
class CatDictionary
{
private:
    string dic_path_;
    boost::unordered_map<std::string, int> catDic_;
public:
    void close()
    {
        std::ofstream ofs(dicPath_, std::ofstream::binary | std::ofstream::trunc);
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

    bool init(string dicpath, bool load = false)
    {
        dicPath_ = (dicpath + "/category_dic.dat");
        if (!load)
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
            return make_pair<std::string, size_t>(h, 0);
        }
    }

    inline int add(const string& cat, int df = 1)
    {
        boost::unordered_map<std::string, int>::iterator iter = catDic_.find(r);
        if (iter != catDic_.end())
        {
            iter->second += df;
            return it->second;
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
            std::ifstream ifs(dic_path.c_str(), std::ios::binary);
            boost::archive::text_iarchive ia(ifs);
            ia >> catDic_;
            return true;
        }
        return false;
    }

    boost::unordered_map<std::string, Category>& getCatDic()
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
