#ifndef AD_SPONSORED_SCORE_MGR_H
#define AD_SPONSORED_SCORE_MGR_H

#include <vector>
#include <map>
#include <deque>
#include <boost/unordered_map.hpp>
#include <string>

namespace sf1r
{

namespace sponsored
{
class AdScoreMgr
{
public:
    typedef uint32_t docid_t;
    void filterByBroadMatch(std::vector<docid_t>& docids, std::vector<double>& rank_scorelist);

};

}


}

#endif
