#include <fstream>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "laser-manager/clusteringmanager/clustering/PCAProcess.h"
#include "Configuration.h"
#include <boost/timer.hpp>
#include <glog/logging.h>

void printHelp()
{
    cout << "./title_pca_util\n"
         << "-c\tcorpus path\n \
		\t-C\tconfig path\n";
}

using namespace sf1r::laser::conf;
using namespace sf1r::laser::clustering;

int main(int argc, char * argv[])
{
    if (argc == 1)
    {
        printHelp();
        return 0;
    }

    char c = '?';
    string corpus;
    string cfgFile;
    while ((c = getopt(argc, argv, "c:C:")) != -1)
    {
        switch (c)
        {
        case 'c':
            corpus = optarg;
            break;
        case 'C':
            cfgFile = optarg;
            break;
        case '?':
            printHelp();
            break;
        }
    }

    if(!Configuration::get()->parse(cfgFile))
    {
        std::cout<<"Error on parse cfgFile:"<<cfgFile<<std::endl;
        return 0;
    }

    string pca_directory_path = Configuration::get()->getDictionaryPath();
    std::cout<<"pca_directory_path:"<<pca_directory_path<<endl;
    float threhold = Configuration::get()->getPcaThrehold();
    
    PCAProcess* pca = new PCAProcess(Configuration::get()->getClusteringRootPath(),
        corpus,
        Configuration::get()->getDictionaryPath(),
        Configuration::get()->getPcaThrehold(),
        Configuration::get()->getMinClusteringDocNum(),
        Configuration::get()->getMaxClusteringDocNum(),
        Configuration::get()->getClusteringResultTermNumLimit(),
        Configuration::get()->getClusteringExecThreadnum());
    pca->run();
    delete pca;
    return 0;
}

