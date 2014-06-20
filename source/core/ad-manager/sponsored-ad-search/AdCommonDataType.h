#ifndef AD_COMMON_DATATYPE_H
#define AD_COMMON_DATATYPE_H

#include <vector>
#include <stdint.h>
#include <string>

namespace sf1r
{
namespace sponsored
{
typedef uint32_t ad_docid_t;
typedef uint32_t BidKeywordId;
typedef std::string BidPhraseStrT;
// each bidphrase has several keywords separated by space.
typedef std::vector<BidKeywordId> BidPhraseT;
// each ad can have several bidphrase
typedef std::vector<BidPhraseT> BidPhraseListT;

typedef std::vector<BidPhraseStrT> BidPhraseStrListT;

}

}

#endif
