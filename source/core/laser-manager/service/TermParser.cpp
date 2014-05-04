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
bool TermParser::init(std::string clusteringRoot, std::string dictionPath )
{
    TermDictionary term_dictionary(clusteringRoot, ONLY_READ);
    terms = term_dictionary.getTerms();
    tok = new TitlePCA(dictionPath);
    return true;
}

SplitTitleResult TermParser::parse(SplitTitle st)
{
    SplitTitleResult splitTitleResult;
    if(tok == NULL)
    {
        cout<<"tok is NULL"<<endl;
        return splitTitleResult;
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
        boost::unordered_map<int, Term>::iterator iter = terms.find(Hash_(tks[i].first));
        if(iter != terms.end())
        {
            splitTitleResult.term_list_[iter->second.index] += tks[i].second;
            total+=tks[i].second;
        }
    }
    if(total != 0)
    {
        for (std::map<int, float>::iterator iter = splitTitleResult.term_list_.begin(); iter != splitTitleResult.term_list_.end(); iter++)
        {
            iter->second /= total;
        }
    }

    return splitTitleResult;
}
}
}
}
}

