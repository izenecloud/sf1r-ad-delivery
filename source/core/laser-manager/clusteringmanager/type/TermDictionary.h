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
    TermDictionary(const std::string& filename)
        : filename_ (filename)
    {
        if (boost::filesystem::exists(filename_))
        {
            load();
        }
    }
    
    ~TermDictionary()
    {
    }

public:    
    void load(const std::string& path = "")
    {
        {
            std::ifstream ifs(filename_.c_str(), std::ios::binary);
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

    bool find(const std::string& term) const
    {
        return terms_.find(term) == terms_.end();
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

    void limit(std::size_t ls)
    {
        if (terms_.size() <= ls)
            return;

        std::vector<std::pair<std::string, std::pair<int, int> > > terms(terms_.begin(),terms_.end());
        std::sort(terms.begin(),terms.end(), compare);
        terms_.clear();

        for (std::size_t i = 0; i < ls; ++i)
        {
            //std::cout<<terms[i].first<<"\t"<<terms[i].second.first<<"\n";
            terms_[terms[i].first] = std::make_pair(terms[i].second.first, terms_.size());
        }
    }

    boost::unordered_map<std::string, std::pair<int, int> >& getTerms()
    {
        return terms_;
    }

    void save()
    {
        LOG(INFO)<<"save term dictionary, term number = "<<terms_.size();
        std::ofstream ofs(filename_ .c_str(), std::ofstream::binary | std::ofstream::trunc);
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
    static bool compare(const std::pair<std::string, std::pair<int, int> >& lv,
        const std::pair<std::string, std::pair<int, int> >& rv)
    {
        return lv.second.first > rv.second.first;
    }
private:
    // first int : count;
    // second int : index
    boost::unordered_map<std::string, std::pair<int, int> > terms_;;
    const std::string filename_;
};

} } } }
#endif /* TERM_DICTIONARY_H_ */
