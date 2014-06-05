#ifndef SF1R_LASER_RECOMMEND_H
#define SF1R_LASER_RECOMMEND_H
#include <string>
#include <vector>
#include <util/functional.h>
#include <common/inttypes.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <3rdparty/msgpack/rpc/server.h>
#include <queue>
namespace sf1r {
class LaserManager;
}

namespace sf1r { namespace laser {
class LaserModel;
class LaserModelFactory;

class LaserRecommend
{
typedef izenelib::util::second_greater<std::pair<docid_t, float> > greater_than;
typedef std::priority_queue<std::pair<docid_t, float>, std::vector<std::pair<docid_t, float> >, greater_than> priority_queue;

public:
    LaserRecommend(const LaserManager* laserManager);
    ~LaserRecommend();
public:
    bool recommend(const std::string& text, 
        std::vector<docid_t>& itemList, 
        std::vector<float>& itemScoreList, 
        const std::size_t num) const;
    
    void dispatch(const std::string& method, msgpack::rpc::request& req);
private:
    void topn(const docid_t& docid, const float score, const std::size_t n, priority_queue& queue) const;
    void extractContext(const std::string& text, std::vector<std::pair<int, float> >& context) const;
private:
    const LaserManager* laserManager_;
    const LaserModelFactory* factory_;
    LaserModel* model_;
};

} }

#endif
