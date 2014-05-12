/*
 * TermParser.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */

#include "TermParser.h"
using namespace sf1r::laser::clustering::type;
using namespace ilplib::knlp;
namespace sf1r { namespace laser { namespace clustering { namespace rpc {
TermParser::TermParser()
    : termDict_(NULL)
    , tok(NULL)
{
}
TermParser::~TermParser()
{
    if (NULL != termDict_)
    {
        delete termDict_;
        termDict_ = NULL;
    }

    if(tok != NULL)
    {
        delete tok;
        tok = NULL;
    }
}
bool TermParser::init(const std::string& workdir, const std::string& dictionPath )
{
    termDict_ = new TermDictionary(workdir);
    termDict_->load();
    tok = new TitlePCA(dictionPath);
    return true;
}

void TermParser::reload(const std::string& clusteringRoot)
{
    boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
    termDict_->load(clusteringRoot);
    // serialization for next start
    termDict_->save();
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
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator iter = termDict_->getTerms().find(tks[i].first);
        if(iter != termDict_->getTerms().end())
        {
            res.term_list_[iter->second.second] += tks[i].second;
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

void TermParser::numeric(const boost::unordered_map<std::string, float>& pow, std::map<int, float>& res) const
{
    boost::unordered_map<std::string, float>::const_iterator it = pow.begin();
    for (; it != pow.end(); ++it)
    {
        boost::unordered_map<std::string, std::pair<int, int> >::const_iterator thisIt = termDict_->getTerms().find(it->first);
        if (termDict_->getTerms().end() != thisIt)
        {
            res[thisIt->second.second] = it->second; 
        }
    }
}


} } } }
