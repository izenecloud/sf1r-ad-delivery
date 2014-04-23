#include "Request.h"

namespace predict
{
    bool Request::parseRequest(const izenelib::driver::Request& req)
    {
        uuid_ = asString(req["uuid"]);
        num_ = asInt(req["topn"]);
        return true;
    }
}
