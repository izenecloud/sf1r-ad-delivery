#ifndef SF1R_LASER_CLUSTERING_SERVER_REQUEST_H_
#define SF1R_LASER_CLUSTERING_SERVER_REQUEST_H_

#include "DataType.h"

namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace rpc
{

class CLUSTERINGServerRequest
{
public:
    typedef std::string method_t;

    /// add method here
    enum METHOD
    {
        METHOD_SPLITETITLE = 0,
        METHOD_GETCLUSTERINGITEMLIST,
        METHOD_GETCLUSTERINGINFOS,
        METHOD_UPDATETOPNCLUSTER,
        METHOD_UPDATEPERUSERMODEL,
        METHOD_TEST,
        COUNT_OF_METHODS
    };

    static const method_t method_names[COUNT_OF_METHODS];

    METHOD method_;

public:
    CLUSTERINGServerRequest(const METHOD& method) : method_(method) {}
    virtual ~CLUSTERINGServerRequest() {}
};

template <typename RequestDataT>
class ClusteringRequestRequestT : public CLUSTERINGServerRequest
{
public:
    ClusteringRequestRequestT(METHOD method)
        : CLUSTERINGServerRequest(method)
    {
    }

    RequestDataT param_;

    MSGPACK_DEFINE(param_)
};


class TestRequest: public ClusteringRequestRequestT<TestData>
{
public:
    TestRequest()
        : ClusteringRequestRequestT<TestData>(METHOD_TEST)
    {
    }
};

class SpliteTtileRequest: public ClusteringRequestRequestT<SplitTitleResult>
{
public:
    SpliteTtileRequest()
        : ClusteringRequestRequestT<SplitTitleResult>(METHOD_SPLITETITLE)
    {
    }
};

} // namespace rpc
} // namespace clustering
}
}
#endif /* _CLUSTERING_SERVER_REQUEST_H_ */
