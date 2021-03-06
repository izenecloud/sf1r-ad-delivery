#include "clustering/Configuration.h"

#include <util/string/StringUtils.h>

#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;
namespace sf1r
{
namespace laser
{
namespace conf
{

static const unsigned int DEFAULT_FLUSH_CHECK_INTERVAL = 60;

Configuration::Configuration()
{
}

Configuration::~Configuration()
{
}

bool Configuration::parse(const std::string& cfgFile)
{
    cfgFile_ = cfgFile;
    return parseCfgFile_(cfgFile);
}

bool Configuration::parseCfgFile_(const std::string& cfgFile)
{
    try
    {
        if (!bfs::exists(cfgFile) || !bfs::is_regular_file(cfgFile))
        {
            std::cerr << "\"" << cfgFile
                      << "\" is not existed or not a regular file." << std::endl;
            return false;
        }

        std::ifstream cfgInput(cfgFile.c_str());
        std::string cfgString;
        std::string line;

        if (!cfgInput.is_open())
        {
            std::cerr << "Could not open file: " << cfgFile << std::endl;
            return false;
        }

        while (getline(cfgInput, line))
        {
            izenelib::util::Trim(line);
            if (line.empty() || line[0] == '#')
            {
                // ignore empty line and comment line
                continue;
            }

            if (!cfgString.empty())
            {
                cfgString.push_back('\n');
            }
            cfgString.append(line);
        }

        // check configuration properties
        properties props('\n', '=');
        props.loadKvString(cfgString);

        parseCfg(props);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

void Configuration::parseCfg(properties& props)
{
    parseServerCfg(props);
}

void Configuration::parseServerCfg(properties& props)
{
    getValue(props, "clustering.dictionarypath", dictionaryPath_);
    getValue(props, "clustering.result.root", clusteringRootPath_);

    getValue(props, "clustering.threhold", pcaThrehold_);

    getValue(props, "clustering.result.mindocnum", minClusteringDocNum_);

    getValue(props, "clustering.result.maxdocnum", maxClusteringDocNum_);

    getValue(props, "clustering.result.termnum", clusteringResultTermNumLimit);

    getValue(props, "clustering.result.mintermnum", clusteringResultMintermnum);

    getValue(props, "clustering.exec.threadnum", clusteringExecThreadnum);
}
}
}
}
