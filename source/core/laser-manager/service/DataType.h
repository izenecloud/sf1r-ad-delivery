#ifndef DATA_TYPE_H_
#define DATA_TYPE_H_

#include <3rdparty/msgpack/msgpack.hpp>
#include "type/Document.h"
#include "common/constant.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <type/ClusteringInfo.h>
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

struct SubscribeData: public RequestData
{
    std::string topic_;
    std::string ip_;
    int port_;
    bool sub_;

    MSGPACK_DEFINE(topic_, ip_, port_, sub_);
};

struct SplitTitle: public RequestData
{
    std::string title_;
    MSGPACK_DEFINE(title_);
};

struct SplitTitleResult
{
    std::map<size_t, float> term_list_;
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
    std::vector<clustering::type::ClusteringInfo> info_list_;
    MSGPACK_DEFINE(info_list_);
};
struct GetClusteringItemResult
{
    std::vector<clustering::type::Document> item_list_;
    MSGPACK_DEFINE(item_list_);
};

} // namespace rpc
} // namespace cluaster
#endif /* DATA_TYPE_H_ */
