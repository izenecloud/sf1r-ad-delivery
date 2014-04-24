#ifndef SF1R_LASER_RECOMMEND_H
#define SF1R_LASER_RECOMMEND_H
#include <string>
#include <vector>

namespace sf1r
{
namespace laser
{
class LaserRecommend
{
public:
    bool recommend(const std::string& uuid, 
        std::vector<std::string>& itemList, 
        std::vector<float>& itemScoreList, 
        const std::size_t num) const;
};
}
}

#endif
