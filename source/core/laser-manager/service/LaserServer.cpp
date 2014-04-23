#include <unistd.h>
#include "LaserProcess.h"
#include "OnSignal.h"
#include "conf/Configuration.h"
#include <glog/logging.h>
using namespace service;
int main(int argc, char* argv[])
{
    RETURN_ON_FAILURE(conf::Configuration::get()->parse(argv[1]));
    setupDefaultSignalHandlers();
    try
    {
        LaserProcess preProcess;
        return preProcess.run();
    }
    catch (const std::exception& e)
    {
        LOG(INFO)<<"Some errors occur, "<<e.what();
    }
    return 0;
}
