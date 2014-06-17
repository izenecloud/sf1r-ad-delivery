#ifndef SF1R_SLIM_RECOMMEND_H
#define SF1R_SLIM_RECOMMEND_H

#include <string>
#include <vector>
#include <common/inttypes.h>
#include <laser-manager/AdIndexManager.h>

namespace sf1r { namespace slim {

class SlimRecommend {
public:
    SlimRecommend(laser::AdIndexManager* index)
        : indexManager_(index)
    {
    }

    bool recommend(const std::string & uuid,
                   int num,
                   std::vector<docid_t> & itemList) const;

private:
    laser::AdIndexManager* indexManager_;
};

}}

#endif
