#ifndef CONF_CONFIGURATION_H
#define CONF_CONFIGURATION_H

#include <string>
#include <set>

#include <util/singleton.h>
#include <util/kv2string.h>
#include <boost/unordered_map.hpp>
#include <glog/logging.h>
namespace sf1r
{
namespace laser
{
namespace conf
{

typedef izenelib::util::kv2string properties;

class Configuration
{
public:
    Configuration();

    ~Configuration();

    static Configuration* get()
    {
        return izenelib::util::Singleton<Configuration>::get();
    }

    bool parse(const std::string& cfgFile);

    inline const std::string& getHost() const
    {
        return host_;
    }

    inline unsigned int getPort() const
    {
        return rpcPort_;
    }

    inline unsigned int getRpcThreadNum() const
    {
        return rpcThreadNum_;
    }
    const std::string& getDictionaryPath() const
    {
        return dictionaryPath_;
    }

    float getPcaThrehold() const
    {
        return pcaThrehold_;
    }

    unsigned int getRpcPort() const
    {
        return rpcPort_;
    }


    const std::string& getClusteringRootPath() const
    {
        return clusteringRootPath_;
    }

    void setClusteringRootPath(const std::string& clusteringRootPath)
    {
        clusteringRootPath_ = clusteringRootPath;
    }

    size_t getMaxClusteringDocNum() const
    {
        return maxClusteringDocNum_;
    }

    void setMaxClusteringDocNum(size_t maxClusteringDocNum)
    {
        maxClusteringDocNum_ = maxClusteringDocNum;
    }

    size_t getMinClusteringDocNum() const
    {
        return minClusteringDocNum_;
    }

    void setMinClusteringDocNum(size_t minClusteringDocNum)
    {
        minClusteringDocNum_ = minClusteringDocNum;
    }

    size_t getClusteringResultTermNumLimit() const
    {
        return clusteringResultTermNumLimit;
    }

    void setClusteringResultTermNumLimit(size_t clusteringResultTermNumLimit)
    {
        this->clusteringResultTermNumLimit = clusteringResultTermNumLimit;
    }

    int getClusteringExecThreadnum() const
    {
        return clusteringExecThreadnum;
    }

    void setClusteringExecThreadnum(int clusteringExecThreadnum)
    {
        this->clusteringExecThreadnum = clusteringExecThreadnum;
    }

    int getClusteringResultMintermnum() const
    {
        return clusteringResultMintermnum;
    }

    void setClusteringResultMintermnum(int clusteringResultMintermnum)
    {
        this->clusteringResultMintermnum = clusteringResultMintermnum;
    }

    unsigned int getClusteringLevelDbCache() const
    {
        return clusteringLevelDBCache;
    }

    std::string getScdFileSuffix()
    {
        return scdFileSuffix;
    }
    /*std::string getLevelDbRoot()
    {
        return leveldbRoot_;
    }
    std::string getClusteringDBPath()
    {
        return clusteringDBPath_;
    }
    std::string getTopNClusterDBPath()
    {
        return topNClusterDBPath_;
    }
    std::string getPerUserDBPath()
    {
        return perUserDBPath_;
    }*/

private:
    bool parseCfgFile_(const std::string& cfgFile);

    void parseCfg(properties& props);

    void parseServerCfg(properties& props);

    inline bool getValue(properties& props, const char* key, unsigned int& value)
    {
        std::string tempValue;
        if (!props.getValue(key, tempValue))
        {
            throw std::runtime_error(
                std::string("Configuration missing property: ")+key);
        }
        value = atoi(tempValue.c_str());
        return true;
    }
    inline bool getValue(properties& props,const char* key, float& value)
    {
        std::string tempValue;
        if (!props.getValue(key, tempValue))
        {
            throw std::runtime_error(
                std::string("Configuration missing property: ")+key);
        }
        value = atof(tempValue.c_str());
        return true;
    }
    inline bool getValue(properties& props,const char* key, std::string& value)
    {
        if (!props.getValue(key, value))
        {
            throw std::runtime_error(
                std::string("Configuration missing property: ") +key);
        }
        return true;
    }
    // void parseConnectionCfg(properties& props);

    std::string cfgFile_;
    std::string host_;
    unsigned int rpcPort_;
    unsigned int rpcThreadNum_;
    //threhold for pca clustering,  T split to (w1,w2,w3,w4),  if (w1+w2)/(w1+w2+w3) > pcathrehold , the w1w2 will be the category name
    float pcaThrehold_;
    //pca dictionary path
    std::string dictionaryPath_;
    //clusteringRootPath_ will be the clustering result path
    std::string clusteringRootPath_;
    //minClusteringDocNum_ is the min document number in a certain clustering
    unsigned int minClusteringDocNum_;
    //maxClusteringDocNum_ is the max document number in a certain clustering
    unsigned int maxClusteringDocNum_;
    //reduce the term dimension to a certain number
    unsigned int clusteringResultTermNumLimit;
    //the min term number in one item
    unsigned int clusteringResultMintermnum;
    //the thread number for clustering
    unsigned int clusteringExecThreadnum;
    //std::string levelDBPath_;
    unsigned int clusteringLevelDBCache;
    //only parse the scd file with scdFileSuffix
    std::string scdFileSuffix;
    /*std::string leveldbRoot_;
    std::string clusteringDBPath_;
    std::string topNClusterDBPath_;
    std::string perUserDBPath_;*/
};

} //namespace conf
}
}
#endif
