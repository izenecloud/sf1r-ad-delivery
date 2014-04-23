#include "Compaign.h"

namespace predict
{
    
    void Compaign::render(izenelib::driver::Value& value) const
    {
        value["title"] = title_;
        value["url"] = url_;
    }
}
