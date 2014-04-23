#ifndef PREDICT_PREDICT_PROCESS_H
#define RPEDICT_PREDICT_PROCEsS_H
#include <util/driver/DriverServer.h>
#include <boost/shared_ptr.hpp>
#include "RpcServer.h"
namespace service
{
    class LaserProcess
    {
    public:
        LaserProcess();
        ~LaserProcess();
    public:
        int run();
        void startDriver();
        void stopDriver();
        void startMsgpack();
        void stopMsgpack();
    private:
        boost::shared_ptr<izenelib::driver::DriverServer> driverServer_;
        boost::scoped_ptr<clustering::rpc::RpcServer> rpcServer_;
    };
}
#endif
