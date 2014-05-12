/*
 * CatDictionary.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */
#include "ClusteringListDes.h"
#include <boost/filesystem.hpp>

namespace sf1r { namespace laser { namespace clustering { namespace type {

const std::string ClusteringListDes::MID_SUFFIX = ".mid";
const std::string ClusteringListDes::RES_SUFFIX = ".res";
const std::string ClusteringListDes::POW_SUFFIX = ".pow";

bool ClusteringListDes::init(const std::string& clustering_dir)
//        STATUS is_rebuild) 
{
    workdir_ = clustering_dir + "/data/";
    if (!boost::filesystem::exists(workdir_))
    {
        boost::filesystem::create_directory(workdir_);
    }
    return true;
    //clustering_res_dir = clustering_dir_+"/data/";
    //cat_file_list_des_filename = clustering_res_dir+"/clustering.des";
    //clustering_file_status = is_rebuild;
    
    //cout<<"ClusteringListDes init"<<endl;
    //if(is_rebuild == TRUNCATED)
    //{
    //    fs::remove_all(clustering_res_dir);
    //}
    //fs::create_directories(clustering_res_dir);
    //load_cats();
    //cout<<"ClusteringListDes init over"<<endl;
    //return true;
}

} } } }
