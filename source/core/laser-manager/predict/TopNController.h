#ifndef PREDICT_TOPN_CONTROLLER_H
#define PREDICT_TOPN_CONTROLLER_H
#include <util/driver/Controller.h>

namespace predict
{
    class TopNController : public izenelib::driver::Controller
    {
    public:
        TopNController(){}
    public:
        void top();
    };
}
#endif
