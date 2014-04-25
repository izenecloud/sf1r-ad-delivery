/*
 * TermParser.h
 *
 *  Created on: Apr 2, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_TERMPARSER_H
#define SF1R_LASER_TERMPARSER_H
#include <string>
#include <knlp/title_pca.h>
#include <util/singleton.h>

#include "laser-manager/clusteringmanager/type/CatDictionary.h"
#include "laser-manager/clusteringmanager/type/TermDictionary.h"
#include "laser-manager/clusteringmanager/type/Term.h"
#include "DataType.h"

namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace rpc
{
class TermParser
{
public:
    TermParser();
    ~TermParser();
    bool init(std::string clusteringRoot, std::string dictionPath);
    void release()
    {
        if(tok != NULL)
        {
            delete tok;
            tok = NULL;
        }
    }
    clustering::rpc::SplitTitleResult parse(clustering::rpc::SplitTitle title);
    static TermParser* get()
    {
        return izenelib::util::Singleton<TermParser>::get();
    }

private:
    ilplib::knlp::TitlePCA* tok; //the pca
    //term_dictionary will maintain the term dictionary, sort and reduce the dimention
    //TermDictionary term_dictionary;
    std::map<clustering::hash_t, clustering::type::Term> terms;
};
}
}
}
}

#endif /* TERMPARSER_H_ */
