#include <util/driver/Keys.h>
#include "Response.h"
namespace predict
{
    Response::Response()
    {
        res_.reserve(DEFAULT_TOP_N);
        success_ = true;
    }

    Response::~Response()
    {
        std::vector<Compaign*>::const_iterator it =  res_.begin();
        for (; it != res_.end(); ++it)
        {
            delete (*it);
        }
    }
    void Response::render(izenelib::driver::Response& res)
    {
        if (success_)
        {
            res.setSuccess();
            renderBody_(res["Compaigns"]);
        }
        else
        {
            res.addError(errorMessage_);
        }
    }
        
    void Response::renderBody_(izenelib::driver::Value& body)
    {
        std::vector<Compaign*>::const_iterator it =  res_.begin();
        for (; it != res_.end(); ++it)
        {
            (*it)->render(body());
        }
    }
    
}
