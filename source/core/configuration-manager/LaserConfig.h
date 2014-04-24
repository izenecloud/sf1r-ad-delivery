#ifndef SF1R_LASER_CONFIG_H
#define SF1R_LASER_CONFIG_H
#include <string>
#include <stdint.h>
namespace sf1r
{
class LaserMsgpackConfig
{
public:
    std::string host;
    uint16_t port;
    uint32_t numThread;
};

class LaserClusteringConfig
{
public:
    float threhold;
    std::string dictionarypath;
    std::string resultRoot;
    int resultMaxDocNum;
    int resultMinDocNum;
    int execThreadNum;
    // leveldbRootPath is the leveldb root path, every db will be the subdirectory of leveldbrootpath
    std::string leveldbRootPath;
    std::string dbClusteringPath;
    std::string dbTopNClusteringPath;
    std::string dbPerUserModelPath;
    std::string clusteringDbCache;
    std::string clusteringScdSuffix;
};
class LaserConfig
{
public:
    LaserMsgpackConfig msgpack;
    LaserClusteringConfig clustering;
};
}
#endif
