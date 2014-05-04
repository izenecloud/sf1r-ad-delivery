/*
 * Terms.h
 *
 *  Created on: Mar 31, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_TERMS_H
#define SF1R_LASER_TERMS_H
#include <string>
#include <ostream>
#include <boost/serialization/access.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/serialization.hpp>
#include "laser-manager/clusteringmanager/common/constant.h"
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{
class Term
{
public:
    std::string term_str;
    int term_df;
    int index;
    Term()
    {
        term_str = "";
        term_df = 0;
        index = -1;
    }
    Term(std::string t, int td, int ind=0)
    {
        term_str = t;
        term_df = td;
        index = ind;
    }
    Term(const Term& t)
    {
        term_str = t.term_str;
        term_df = t.term_df;
        index = t.index;
    }
    int getTermDf() const
    {
        return term_df;
    }

    friend ostream& operator<< (ostream& os, Term& term)
    {
        os<<"TermStr:"<<term.term_str<<" term_df:"<<term.term_df<< " index:"<<term.index;
        return os;
    }

    void setTermDf(int termDf)
    {
        term_df = termDf;
    }
    static bool compare( pair<hash_t,Term> i, pair<hash_t, Term> j)
    {
        return (i.second.term_df > j.second.term_df);
    }

    const std::string& getTermStr() const
    {
        return term_str;
    }

    void setTermStr(const std::string& termStr)
    {
        term_str = termStr;
    }
    
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & term_str;
        ar & term_df;
        ar & index;
    }
};
}
}
}
}
#endif /* TERMS_H_ */
