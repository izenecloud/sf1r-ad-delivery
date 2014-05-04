/*
 * TermParser.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */

#include "TermParser.h"
using namespace sf1r::laser::clustering::type;
using namespace ilplib::knlp;
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace rpc
{
TermParser::TermParser()
{
    tok = NULL;
}
TermParser::~TermParser()
{
    release();
}
bool TermParser::init(const std::string& clusteringRoot, const std::string& dictionPath )
{
    TermDictionary term_dictionary(clusteringRoot, ONLY_READ);
    terms = term_dictionary.getTerms();
    tok = new TitlePCA(dictionPath);
    return true;
}

void TermParser::reload(const std::string& clusteringRoot)
{
    TermDictionary term_dictionary(clusteringRoot, ONLY_READ);
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    terms = term_dictionary.getTerms();
}

void TermParser::parse(const clustering::rpc::SplitTitle& st, 
    clustering::rpc::SplitTitleResult& res) const
{
    boost::shared_lock<boost::shared_mutex> sharedLock(mutex_, boost::try_to_lock);
    if (!sharedLock)
    {
        return;
    }

    if(tok == NULL)
    {
        cout<<"tok is NULL"<<endl;
        return;
    }

    typedef std::pair<std::string, float> TermPair;
    std::vector< TermPair > tks;
    TermPair tmp;
    std::vector<TermPair > subtks;
    std::string brand, mdt;
    tok->pca(st.title_, tks, brand, mdt, subtks, false);
    float total = 0;
    for (size_t i = 0; i < tks.size(); ++i)
    {
        boost::unordered_map<clustering::hash_t, Term>::const_iterator iter = terms.find(Hash_(tks[i].first));
        if(iter != terms.end())
        {
            res.term_list_[iter->second.index] += tks[i].second;
            total+=tks[i].second;
        }
    }
    
    if(total != 0)
    {
        for (std::map<int, float>::iterator iter = res.term_list_.begin(); iter != res.term_list_.end(); iter++)
        {
            iter->second /= total;
        }
    }

}
}
}
}
}

