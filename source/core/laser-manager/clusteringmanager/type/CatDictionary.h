/*
 * catdictionary.h
 *
 *  Created on: Mar 28, 2014
 *      Author: alex
 */

#ifndef CATDICTIONARY_H_
#define CATDICTIONARY_H_
#include <string>
#include <knlp/title_pca.h>
#include <am/sequence_file/ssfr.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <map>
#include <iostream>
#include <util/singleton.h>
#include "laser-manager/clusteringmanager/common/constant.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include <iostream>
//#include <ifstream>

namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{
struct Category
{
    string value;
    size_t df;
    Category(string v, size_t d)
    {
        value = v;
        df = d;
    }
    Category()
    {
        value = "";
        df = 0;
    }
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & value;
        ar & df;
    }
};
/**
 * CatDictionary store the final category information,
 * It contains a file which record a list of catfile (catfile contains the clusteringresult in a certain orignal cat)
 *
 */
class CatDictionary
{
private:
    /**
     *
     */
    string dic_path_;
    std::map<hash_t, Category> cat_dic;
    STATUS status_;
public:
    /**
     *
     */
    CatDictionary() :
        status_(ONLY_READ)
    {
    }
    ~CatDictionary()
    {

    }
    void close()
    {
        string bak = dic_path_ + ".bak";
        izene_writer* cat_writer_ = openFile<izene_writer>(bak, true);
        cout << "save CatDictionary to local" << endl;
        for (std::map<hash_t, Category>::iterator iter = cat_dic.begin();
                iter != cat_dic.end(); iter++)
        {
            cat_writer_->Append(iter->first, iter->second);
        }
        cout << "save and rename CatDictionary to local" << endl;
        closeFile<izene_writer>(cat_writer_);
        fs::rename(bak, dic_path_);
    }

    inline static CatDictionary* get()
    {
        return izenelib::util::Singleton<CatDictionary>::get();
    }

    bool init(string dicpath, STATUS status = TRUNCATED)
    {
        dic_path_ = (dicpath + "/category_dic.dat");
        status_ = status;
        if (status == TRUNCATED)
        {
            ofstream is(dic_path_.c_str(), ios::trunc);
            is.close();
        }
        bool result = load_dic(dic_path_);
        //bool result2 = cat_writer_ = openFile<izene_writer>(dic_path_, true);
        return result;
    }

    inline pair<hash_t, size_t> getCatHash(const string& cat)
    {
        hash_t h = Hash_(cat);
        std::map<hash_t, Category>::iterator iter = cat_dic.find(h);
        if (iter != cat_dic.end())
        {
            return make_pair<hash_t, size_t>(h, iter->second.df);
        }
        else
        {
            return make_pair<hash_t, size_t>(h, 0);
        }
    }

    inline pair<hash_t, size_t> addCatHash(const string& cat, hash_t r = 0, int df = 1)
    {
        if (r == 0)
            r = Hash_(cat);
        std::map<hash_t, Category>::iterator iter = cat_dic.find(r);
        if (iter != cat_dic.end())
        {
            if (status_ != ONLY_READ)
            {
                iter->second.df += df;
            }
            return make_pair<hash_t, size_t>(r, iter->second.df);
        }
        else
        {
            cat_dic.insert(make_pair<hash_t, Category>(r, Category(cat, df)));
            return make_pair<hash_t, size_t>(r, 1);
        }
    }

    bool load_dic(const std::string& dic_path)
    {
        izene_reader_pointer reader = openFile<izene_reader>(dic_path, false);
        hash_t h;
        Category v;
        if (reader == NULL)
        {
            return false;
        }
        else
        {
            while (reader->Next(h, v))
            {
                cat_dic.insert(make_pair<hash_t, Category>(h, v));
            }
            closeFile<izene_reader>(reader);
        }
        return true;
    }

    std::map<hash_t, Category> getCatDic()
    {
        return cat_dic;
    }/*

	 izene_writer* getCatWriter()   {
	 return cat_writer_;
	 }

	 void setCatWriter( izene_writer* catWriter) {
	 cat_writer_ = catWriter;
	 }*/

    string getDicPath() const
    {
        return dic_path_;
    }

    void setDicPath(string dicPath)
    {
        dic_path_ = dicPath;
    }
};

}
}
}
}
#endif /* CATDICTIONARY_H_ */
