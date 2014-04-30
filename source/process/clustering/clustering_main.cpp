/*
 * test_pca.cpp
 *
 *  Created on: Mar 24, 2014
 *      Author: alex
 */
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "knlp/title_pca.h"
#include <map>
#include "laser-manager/clusteringmanager/clustering/PCAClustering.h"
#include "laser-manager/clusteringmanager/clustering/OriDocument.h"
#include "Configuration.h"
#include "laser-manager/clusteringmanager/type/ClusteringListDes.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataAdapter.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataStorage.h"
#include <boost/timer.hpp>
using namespace std;
using namespace ilplib::knlp;
using namespace izenelib;
using namespace sf1r::laser::clustering;
using namespace sf1r::laser::conf;
using namespace sf1r::laser::clustering::type;

void printHelp()
{
    cout << "./title_pca_util\n"
         << "-c\tcorpus path\n \
		\t-C\tconfig path\n";
}
vector<string> getSCDList(string dic, string suffix)
{
    vector<string> filelist;
    fs::path full_path(dic);
    if(fs::exists(full_path))
    {
        fs::directory_iterator item_begin(full_path);
        fs::directory_iterator item_end;
        for ( ; item_begin !=  item_end; item_begin ++ )
        {
            if (fs::is_directory( *item_begin))
            {
            }
            else
            {
                string filename = item_begin->path().c_str();
                cout<<filename<<endl;
                if(filename.length() - filename.find_last_of(suffix) == 1 )
                {
                    cout<<"To load data:"<<filename<<endl;
                    filelist.push_back(filename);
                }
            }
        }
    }
    return filelist;
}

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
    string title;
    string category;
    string docid;

    string pca_directory_path = Configuration::get()->getDictionaryPath();
    std::cout<<"pca_directory_path:"<<pca_directory_path<<endl;
    float threhold = Configuration::get()->getPcaThrehold();
    string clustering_root = Configuration::get()->getClusteringRootPath();
    int min_clustering_doc_num = Configuration::get()->getMinClusteringDocNum();
    int max_clustering_doc_num = Configuration::get()->getMaxClusteringDocNum();
    int max_clustering_term_num = Configuration::get()->getClusteringResultTermNumLimit();
    int clustering_exec_threadnum = Configuration::get()->getClusteringExecThreadnum();
    fs::create_directories(clustering_root);
    //string leveldbpath =  Configuration::get()->getClusteringDBPath();
    string scdsuffix = Configuration::get()->getScdFileSuffix();
    RETURN_ON_FAILURE(ClusteringDataStorage::get()->init(clustering_root));
    vector<string> filelist = getSCDList(corpus, scdsuffix);
    PCAClustering pca_clustering(pca_directory_path, clustering_root, threhold, min_clustering_doc_num, max_clustering_doc_num, max_clustering_term_num);
    namespace pt = boost::posix_time;
    pt::ptime beg = pt::microsec_clock::local_time();
    long count  = 0;
    for(vector<string>::iterator iter = filelist.begin(); iter != filelist.end(); iter++)
    {
        ifstream scdfile((*iter).c_str());
        string line;
        map<string, int> wdt;

        while(  std::getline(scdfile, line) )
        {
            int len = line.length();
            if (len > 7 && line.substr(0, 7) == "<DOCID>")
            {
                if (title.compare("") != 0 && docid.compare("") != 0
                        && category.compare("") != 0)
                {
                    //it's a complete
                    pca_clustering.next(title, category, docid);
                    count++;
                    docid = line.substr(7);
                    if(count %1000 == 0)
                    {
                        std::cout<<"Read:"<<count<<endl;
                    }
                }
                else
                {
                    docid = line.substr(7);
                    title = "";
                    category = "";
                }
            }
            else if(len > 7 && line.substr(0, 7) == "<Title>")
            {
                title = line.substr(7);
            }
            else if(len > 10 && line.substr(0, 10) == "<Category>")
            {
                category = line.substr(10);
            }
            else
            {
                continue;
            }

        }
    }
    pca_clustering.execute(ClusteringDataStorage::get(), clustering_exec_threadnum);
    pt::ptime end = pt::microsec_clock::local_time();
    pt::time_duration diff = end - beg;
    cout<<"cost:"<< diff.total_milliseconds()<<" docnum:"<<count<<endl;

    ClusteringDataStorage::get()->release();
}

