/*
 * term_dictionary.h
 *
 *  Created on: Apr 1, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_TERM_DICTIONARY_H_
#define SF1R_LASER_TERM_DICTIONARY_H__
#include <string>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <stack>
#include <algorithm>
#include <boost/unordered_map.hpp>

#include "Term.h"
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
    //get the index id, the index begin with 1, so if the return is 0, it means there is no result
    bool get(const std::string& term, Term& t) const
    {
        if(term.length() == 1)
            return 0;
        hash_t th = Hash_(term);
        boost::unordered_map<hash_t, Term>::const_iterator iter = terms_.find(th);
        if(iter == terms_.end())
        {
            return false;
        }
        t = iter->second;
        return true;
    }

    void set(const std::string& term, int count)
    {
        hash_t th = Hash_(term);
        boost::unordered_map<hash_t, Term>::iterator iter = terms_.find(th);
        if(iter == terms_.end())
        {
            Term t(term, count, terms_.size());
            terms_.insert(std::make_pair(th, t));
        }
        else
        {
            iter->second.term_df += count;
        }
         
    }

    void sort(size_t limit= 0)
    {
        std::vector<std::pair<hash_t,Term> > terms(terms_.begin(),terms_.end());
        std::sort(terms.begin(),terms.end(), Term::compare);
        std::vector<std::pair<hash_t,Term> > toMove;
        //index begin with 1!!!
        size_t count = 1;
        for(vector< pair<hash_t,Term> >::iterator iter = terms.begin(); iter != terms.end(); iter++)
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
        }
    }

    boost::unordered_map<hash_t, Term>& getTerms()
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

    ~TermDictionary()
    {
    }

private:
    boost::unordered_map<hash_t, Term> terms_;;
    const std::string workdir_;
};

} } } }
#endif /* TERM_DICTIONARY_H_ */
