#ifndef SF1R_LASER_DATA_TYPE_H_
#define SF1R_LASER_DATA_TYPE_H_

#include <3rdparty/msgpack/msgpack.hpp>
#include "laser-manager/clusteringmanager/type/Document.h"
#include "laser-manager/clusteringmanager/common/constant.h"
#include "laser-manager/clusteringmanager/type/ClusteringInfo.h"
#include <string>
#include <vector>
#include <map>
#include <list>
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace rpc
{

inline std::ostream& operator<<(std::ostream& os, uint128_t uint128)
{
    os << hex << uint64_t(uint128>>64) << uint64_t(uint128) << dec;
    return os;
}

struct RequestData
{
};

struct TestData: public RequestData
{
    std::vector<int> test_;

    MSGPACK_DEFINE(test_);
};

struct SplitTitle: public RequestData
{
    std::string title_;
    MSGPACK_DEFINE(title_);
};

struct SplitTitleResult
{
    std::map<int, float> term_list_;
    MSGPACK_DEFINE(term_list_);
};
struct GetClusteringItemListRequest: public RequestData
{
    clustering::hash_t clusteringhash_;
    MSGPACK_DEFINE(clusteringhash_);
};

struct GetClusteringInfoRequest: public RequestData
{
    clustering::hash_t clusteringhash_;
    MSGPACK_DEFINE(clusteringhash_);
};

struct GetClusteringInfosResult
{
    std::vector<std::map<int, float> > info_list_;
    MSGPACK_DEFINE(info_list_);
};
struct GetClusteringItemResult
{
    std::vector<clustering::type::Document> item_list_;
    MSGPACK_DEFINE(item_list_);
};

} // namespace rpc
} // namespace cluaster
}
}
#endif /* DATA_TYPE_H_ */
