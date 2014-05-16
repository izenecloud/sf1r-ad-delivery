#ifndef AD_COMMON_DATATYPE_H
#define AD_COMMON_DATATYPE_H

#include <vector>
#include <stdint.h>

namespace sf1r
{
namespace sponsored
{
typedef uint32_t ad_docid_t;
typedef uint32_t BidKeywordId;
typedef std::vector<BidKeywordId> BidPhraseT;

}

}

#endif
