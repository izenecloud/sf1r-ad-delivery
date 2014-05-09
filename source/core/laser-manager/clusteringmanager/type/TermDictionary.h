/*
 * term_dictionary.h
 *
 *  Created on: Apr 1, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_TERM_DICTIONARY_H
#define SF1R_LASER_TERM_DICTIONARY_H
#include <string>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <stack>
#include <algorithm>
#include <boost/unordered_map.hpp>

#include "laser-manager/clusteringmanager/common/constant.h"
#include "laser-manager/clusteringmanager/common/utils.h"

#include <string>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <glog/logging.h>

namespace sf1r { namespace laser { namespace clustering { namespace type {

class TermDictionary
{
public:
    TermDictionary(const std::string& workdir)
        : workdir_ (workdir + "/terms/")
    {
        if (!boost::filesystem::exists(workdir_))
        {
            boost::filesystem::create_directory(workdir_);
        }
    }
    
    ~TermDictionary()
    {
    }

public:    
    void load(const std::string& dictPath = "")
    {
        LOG(INFO)<<"load term dictionary from: " <<dictPath;
        if (dictPath.empty() && boost::filesystem::exists((workdir_ + "/dic.dat").c_str()))
        {
            std::ifstream ifs((workdir_ + "/dic.dat").c_str(), std::ios::binary);
            boost::archive::text_iarchive ia(ifs);
            ia >> terms_;
        }
        else if (boost::filesystem::exists((dictPath + "/terms/dic.dat").c_str()))
        {
            std::ifstream ifs((dictPath + "/terms/dic.dat").c_str(), std::ios::binary);
            boost::archive::text_iarchive ia(ifs);
            ia >> terms_;
        }
        LOG(INFO)<<"terms number = "<<terms_.size();
    }
    //get the index id, the index begin with 0
    bool get(const std::string& term, std::pair<int, int>& t) const
    {
        if(term.length() == 1)
            return false;
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator iter = terms_.find(term);
        if(iter == terms_.end())
        {
            return false;
        }
        t = iter->second;
        return true;
    }

    void set(const std::string& term, int count)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::iterator iter = terms_.find(term);
        if(iter == terms_.end())
        {
            terms_[term] = std::make_pair(count, terms_.size());
        }
        else
        {
            iter->second.first += count;
        }
         
    }

    void sort(size_t limit= 0)
    {
        /*std::vector<std::pair<std::string,Term> > terms(terms_.begin(),terms_.end());
        std::sort(terms.begin(),terms.end(), Term::compare);
        std::vector<std::pair<std::string,Term> > toMove;
        //index begin with 1!!!
        size_t count = 1;
        for(vector< pair<std::string,Term> >::iterator iter = terms.begin(); iter != terms.end(); iter++)
        {
            if(limit != 0 && count > limit)
            {
                terms_.erase(iter->first);
            }
            else
            {
                terms_[iter->first].index = count;
            }
            count++;
        }*/
    }

    boost::unordered_map<std::string, std::pair<int, int> >& getTerms()
    {
        return terms_;
    }

    void save()
    {
        LOG(INFO)<<"save term dictionary, term number = "<<terms_.size();
        std::ofstream ofs((workdir_ + "/dic.dat").c_str(), std::ofstream::binary | std::ofstream::trunc);
        boost::archive::text_oarchive oa(ofs);
        try
        {
            oa << terms_;
        }
        catch(std::exception& e)
        {
            LOG(INFO)<<e.what();
        }
        ofs.close();
    }

private:
    boost::unordered_map<std::string, std::pair<int, int> > terms_;;
    const std::string workdir_;
};

} } } }
#endif /* TERM_DICTIONARY_H_ */
