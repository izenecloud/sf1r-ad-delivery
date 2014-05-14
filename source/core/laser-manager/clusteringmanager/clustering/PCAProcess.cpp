/*
 * pcaclustering.cpp
 *
 *  Created on: Mar 27, 2014
 *      Author: alex
 */

#include <string>
#include <vector>
#include <sstream>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <limits>
#include "PCAProcess.h"

using namespace sf1r::laser::clustering::type;
using namespace std;
using namespace boost::filesystem;
    
namespace sf1r { namespace laser { namespace clustering {

PCAProcess::PCAProcess(const std::string& workdir, 
        const std::string& scddir, 
        const std::string& pcaDictPath,
        float threhold,  
        const std::size_t minDocPerClustering, 
        const std::size_t maxDocPerClustering, 
        const std::size_t dictLimit, 
        const std::size_t threadNum)
   : workdir_(workdir)
   , scddir_(scddir)
   , dictLimit_(dictLimit)
   , termDict_(workdir_)
   , runner_(threadNum, termDict_, pcaDictPath, threhold, maxDocPerClustering, minDocPerClustering)
{
    if (!exists(workdir_))
    {
        create_directory(workdir_);
    }
}

PCAProcess::~PCAProcess()
{
}

void PCAProcess::run()
{
    LOG(INFO)<<"prepare to clustering, statistics...";
    initFstream();
    runner_.startStatistics();
    Document doc;
    while (next(doc.title, doc.category))
    {
        runner_.push_back(doc);
    }
    runner_.stopStatistics();
    LOG(INFO)<<"statistics finish";
    termDict_.limit(dictLimit_);

    LOG(INFO)<<"clustering...";
    initFstream();
    runner_.startClustering();
    while (next(doc.title, doc.category))
    {
        runner_.push_back(doc);
    }
    runner_.stopClustering();
    LOG(INFO)<<"clustering finish";
    
    save();
}

void PCAProcess::save()
{
    termDict_.save();
    const PCARunner::ClusteringContainer& clusteringResult = runner_.getClusteringResult();
    LOG(INFO)<<"clustering summary:\n\tclustering size = "<<clusteringResult.size();
    {
        std::ofstream ofs((workdir_ + "/clustering").c_str(), std::ofstream::binary | std::ofstream::trunc);
        boost::archive::text_oarchive oa(ofs);
        try
        {
            oa << clusteringResult;
        }
        catch(std::exception& e)
        {
            LOG(INFO)<<e.what();
        }
        ofs.close();
    }
}

bool PCAProcess::next(std::string& title, std::string& category)
{
    while (!fstream_.empty())
    {
        fstream* stream = fstream_.front();
        string line = "";
        string docid = ""; 
        while(std::getline(*stream, line) )
        {
            int len = line.length();
            if (len > 7 && line.substr(0, 7) == "<DOCID>")
            {
                if (title.compare("") != 0 && docid.compare("") != 0
                        && category.compare("") != 0)
                {
                    return true;
                    //it's a complete
                    //count++;
                    //docid = line.substr(7);
                    //if(count %1000 == 0)
                    //{
                    //    std::cout<<"Read:"<<count<<endl;
                    //}
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
        // end of file
        delete stream;
        fstream_.pop();
    }
    return false;
}

void PCAProcess::initFstream()
{
    path scddir(scddir_);
    if(exists(scddir))
    {
        directory_iterator it(scddir);
        for ( ; it !=  directory_iterator(); ++it)
        {
            if (!is_directory( *it))
            {
                string filename = it->path().c_str();
                if(filename.length() - filename.find_last_of("SCD") == 1 )
                {
                    LOG(INFO)<<"To load data:"<<filename<<endl;
                    fstream_.push(new fstream(filename.c_str()));
                }
            }
        }
   }
}
} } }
