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

namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{


class TermDictionary
{
private:
    string dictionary_dir_path;
    string dictionary_path;

    //fstream dictionary_file;
    STATUS status;
    //int dictionary_limit;
    boost::unordered_map<hash_t, Term> term_map;
public:
    TermDictionary(string clustering_path, STATUS s)
        :dictionary_dir_path(clustering_path+"/terms/"),
        dictionary_path(dictionary_dir_path+"dic.dat"), status(s)
    {
        cout<<"TermDictionary init"<<endl;

        fs::create_directories(dictionary_dir_path);
        load();
        cout<<"TermDictionary init over"<<endl;
    }
    
    void load()
    {
        if(status == TRUNCATED)
        {
            fstream dictionary_file(dictionary_path.c_str(),ios::out);
            dictionary_file.close();
        }
        else
        {
            izene_reader_pointer iz = openFile<izene_reader>(dictionary_path, false);
            if(iz == NULL)
                return;
            hash_t key;
            Term value;
            while(iz->Next(key, value))
            {
                boost::unordered_map<hash_t, Term>::iterator iter = term_map.find(key);
                if(iter == term_map.end())
                {
                    term_map.insert(make_pair<hash_t, Term> (key, value));
                }
                else
                {
                }
            }
            closeFile<izene_reader>(iz);
        }
    }
    //get the index id, the index begin with 1, so if the return is 0, it means there is no result
    hash_t get(string term, int count=1)
    {
        if(term.length() == 1)
            return 0;
        hash_t th = Hash_(term);
        boost::unordered_map<hash_t, Term>::iterator iter = term_map.find(th);
        if(iter == term_map.end())
        {
            if(status != ONLY_READ)
            {
                Term t(term, count, term_map.size());
                term_map.insert(make_pair<hash_t, Term>(th, t));
            }
            else
            {
                th = 0;
            }
        }
        else
        {
            if(status != ONLY_READ)
            {
                iter->second.term_df+=count;
            }
        }
        return th;
    }

    void sort(size_t limit= 0)
    {
        if(status == ONLY_READ)
            return;
        vector<pair<hash_t,Term> > terms(term_map.begin(),term_map.end());
        std::sort(terms.begin(),terms.end(), Term::compare);
        vector<pair<hash_t,Term> > toMove;
        //index begin with 1!!!
        size_t count = 1;
        for(vector< pair<hash_t,Term> >::iterator iter = terms.begin(); iter != terms.end(); iter++)
        {
            if(limit != 0 && count > limit)
            {
                term_map.erase(iter->first);
            }
            else
            {
                term_map[iter->first].index = count;
            }
            count++;
        }
    }

    void output()
    {
        for(boost::unordered_map<hash_t,Term>::iterator iter = term_map.begin(); iter != term_map.end(); iter++)
        {
            cout<<iter->first<<" "<<iter->second<<endl;
        }
    }

    boost::unordered_map<hash_t, Term> getTerms()
    {
        return term_map;
    }

    void save()
    {
        string filebak =  dictionary_path+".bak";
        cout<<"to writer to :"<<filebak<<" term_map size"<<term_map.size()<< endl;
        izene_writer_pointer izw = openFile<izene_writer>(filebak, true);
        for(boost::unordered_map<hash_t,Term>::iterator iter = term_map.begin(); iter != term_map.end(); iter++)
        {
            izw->Append(iter->first, iter->second);
        }
        closeFile<izene_writer>(izw);
        fs::rename(filebak, dictionary_path);
    }

    ~TermDictionary()
    {
        if(status != ONLY_READ)
            save();
    }

    string  getDictionaryPath()
    {
        return dictionary_path;
    }

    void setDictionaryPath(  string  dictionaryPath)
    {
        dictionary_path = dictionaryPath;
    }

    STATUS getStatus() const
    {
        return status;
    }

    void setStatus(STATUS status)
    {
        this->status = status;
    }

    boost::unordered_map<hash_t, Term>& getTermMap()
    {
        return term_map;
    }

    void setTermMap(boost::unordered_map<hash_t, Term>& termMap)
    {
        term_map = termMap;
    }
};

}
}
}
}
#endif /* TERM_DICTIONARY_H_ */
