/*
 * Document.h
 *
 *  Created on: Mar 31, 2014
 *      Author: alex
 */

#ifndef DOCUMENT_H_
#define DOCUMENT_H_
#include<string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <3rdparty/msgpack/msgpack.hpp>
#include "common/constant.h"

namespace clustering
{
namespace type
{
class Document
{
public:
    Document(std::string id="")
    {
        doc_id = id;
    }
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & doc_id;
        ar & terms;
    }
    void add(hash_t term, float value)
    {
        terms[term] = value;
    }

    std::string doc_id;
    typedef  std::map<clustering::hash_t, float> TermMapType;
    TermMapType terms;
    MSGPACK_DEFINE(doc_id, terms);
};
}
}
#endif /* DOCUMENT_H_ */
