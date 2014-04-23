#ifndef PREDICT_COMPAIGN_H
#define PREDICT_COMPAIGN_H

#include <util/driver/Value.h>

namespace predict
{
    class Compaign
    {
    public:
        const std::string& getTitle() const
        {
            return title_;
        }

        const std::string& getUrl() const
        {
            return url_;
        }


        void render(izenelib::driver::Value& value) const;
    private:
        std::string title_;
        std::string url_;
    };
}

#endif
