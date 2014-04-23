/*
 * TermParser.h
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */

#ifndef TERMPARSER_H_
#define TERMPARSER_H_
#include <string>
#include "knlp/title_pca.h"
#include "type/CatDictionary.h"
#include "type/TermDictionary.h"
#include "DataType.h"
#include "type/Term.h"
#include <util/singleton.h>

using namespace clustering::type;
namespace clustering
{
namespace rpc
{
class TermParser
{
public:
    TermParser();
    ~TermParser();
    bool init(const std::string& clusteringRootPath);
    void release()
    {
        if(tok != NULL)
        {
            delete tok;
            tok = NULL;
        }
    }
    SplitTitleResult parse(SplitTitle title);
    static TermParser* get()
    {
        return izenelib::util::Singleton<TermParser>::get();
    }

private:
    TitlePCA* tok; //the pca
    //term_dictionary will maintain the term dictionary, sort and reduce the dimention
    //TermDictionary term_dictionary;
    std::map<hash_t, Term> terms;
};
}
}

#endif /* TERMPARSER_H_ */
