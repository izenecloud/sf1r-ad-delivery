#ifndef SF1R_SLIM_RECOMMEND_H
#define SF1R_SLIM_RECOMMEND_H

#include <string>
#include <vector>
#include <common/inttypes.h>

namespace sf1r { namespace slim {

class SlimRecommend {
public:
    bool recommend(const std::string & uuid,
                   int num,
                   std::vector<docid_t> & itemList) const;
};

}}

#endif
