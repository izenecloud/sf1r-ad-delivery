#include "LaserProcess.h"
#include "RouterInitializer.h"
#include "OnSignal.h"
#include "conf/Configuration.h"
#include "util/driver/DriverServer.h"
#include <boost/scoped_ptr.hpp>
#include <glog/logging.h>

using namespace izenelib::driver;
using namespace boost;

namespace service
{
LaserProcess::LaserProcess()
{
    startDriver();
    startMsgpack();
}

LaserProcess::~LaserProcess()
{
}

int LaserProcess::run()
{
    try
    {
        rpcServer_->start();
        LOG(INFO)<<"Msgpack service has started...";
        LOG(INFO)<<"Predict service has started...";
        driverServer_->run();
        LOG(INFO)<<"Predict service has stopped";
        waitSignalThread();
    }
    catch (std::exception& e)
    {
        LOG(INFO)<<"Some errors occur, "<<e.what();
        return -1;
    }
    return 0;
}

void LaserProcess::startDriver()
{
    //TODO
    std::size_t threadPoolSize = 4;
    unsigned int port = 6688;

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(),port);
    shared_ptr<Router> driverRouter(new Router());
    initializeRouter(*driverRouter);

    boost::shared_ptr<DriverConnectionFactory> factory(
        new DriverConnectionFactory(driverRouter)
    );

    driverServer_.reset(
        new DriverServer(endpoint, factory, threadPoolSize)
    );

    addExitHook(bind(&LaserProcess::stopDriver, this));
}

void LaserProcess::stopDriver()
{
    LOG(INFO)<<"stoping driver...";
    driverServer_->stop();
}

void LaserProcess::startMsgpack()
{
    rpcServer_.reset(
        new clustering::rpc::RpcServer(
            conf::Configuration::get()->getHost(),
            conf::Configuration::get()->getPort(),
            conf::Configuration::get()->getRpcThreadNum()
        ));

    rpcServer_->init();
    addExitHook(bind(&LaserProcess::stopMsgpack, this));
}
void LaserProcess::stopMsgpack()
{
    LOG(INFO)<<"stoping msgpack..";
    rpcServer_->stop();
}
}
