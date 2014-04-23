/*
 * TermParser.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */

#include "TermParser.h"
#include "conf/Configuration.h"
using namespace conf;
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
bool TermParser::init(const std::string& clusteringRootPath)
{
    TermDictionary term_dictionary(clusteringRootPath, ONLY_READ);
    terms = term_dictionary.getTerms();
    tok = new TitlePCA(Configuration::get()->getDictionaryPath());
    return true;
}
SplitTitleResult TermParser::parse(SplitTitle st)
{
    SplitTitleResult splitTitleResult;
    cout<<"Toparse:"<<st.title_<<endl;
    if(tok == NULL)
    {
        cout<<"tok is NULL"<<endl;
        return splitTitleResult;
    }

    std::vector<std::pair<std::string, float> > tks;
    std::pair<std::string, float> tmp;
    std::vector<std::pair<std::string, float> > subtks;
    std::string brand, mdt;
    tok->pca(st.title_, tks, brand, mdt, subtks, false);

    for (size_t i = 0; i < tks.size(); ++i)
    {
        map<hash_t, Term>::iterator iter = terms.find(Hash_(tks[i].first));
        if(iter != terms.end())
        {
            cout<<"insert "<<iter->second.index <<" float:"<<tks[i].second<<endl;
            splitTitleResult.term_list_.insert(make_pair<hash_t,float>( iter->second.index, tks[i].second) );
        }
    }
    return splitTitleResult;
}
}
}

