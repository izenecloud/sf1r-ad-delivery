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
#include <boost/thread/mutex.hpp>

#include "laser-manager/clusteringmanager/type/CatDictionary.h"
#include "laser-manager/clusteringmanager/type/TermDictionary.h"
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
    
    static TermParser* get()
    {
        return izenelib::util::Singleton<TermParser>::get();
    }

    bool init(const std::string& workdir, const std::string& dictionPath);
    void reload(const std::string& clusteringRoot);
    void parse(const clustering::rpc::SplitTitle& title, clustering::rpc::SplitTitleResult& res) const;
    
private:
    ilplib::knlp::TitlePCA* tok; //the pca
    //term_dictionary will maintain the term dictionary, sort and reduce the dimention
    //TermDictionary term_dictionary;
    type::TermDictionary* termDict_;
    mutable boost::shared_mutex mutex_;
};
}
}
}
}

#endif /* TERMPARSER_H_ */
