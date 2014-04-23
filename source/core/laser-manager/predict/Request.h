#ifndef PREDICT_REQUEST_H
#define PREDICT_REQUEST_H

#include <string>
#include <util/driver/Request.h>
namespace predict
{
    class Request
    {
    public:
        const std::string& getUuid() const
        {
            return uuid_;
        }

        const int getTopN() const
        {
            return num_;
        }

        bool parseRequest(const izenelib::driver::Request& request);
    private:
        std::string uuid_;
        int num_;
    };
}

#endif
