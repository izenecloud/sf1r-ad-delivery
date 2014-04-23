#ifndef PREDICT_RESPONSE_H
#define PREDICT_RESPONSE_H
#include <util/driver/Response.h>
#include <util/driver/Value.h>
#include <vector>

#include "Compaign.h"

namespace predict
{
    class Response
    {
    public:
        Response();
        ~Response();
    public:
        void render(izenelib::driver::Response& res);

        void errorMessage(const std::string& errorMsg)
        {
            errorMessage_ = errorMsg;
        }

        const std::string& errorMessage()
        {
            return errorMessage_;
        }

        void push_back(Compaign* compaign)
        {
            res_.push_back(compaign);
        }
    private:
        void renderBody_(izenelib::driver::Value& body);
    private:
        std::vector<Compaign*> res_;
        std::string errorMessage_;
        bool success_;
        const static int DEFAULT_TOP_N = 8;
    };
}

#endif
