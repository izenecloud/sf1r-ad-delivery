/*
 * CatDictionary.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */
#include "ClusteringListDes.h"
namespace sf1r
{
namespace laser
{
namespace clustering
{
namespace type
{

bool ClusteringListDes::init(
        string clustering_dir_, 
        STATUS is_rebuild, 
        string mid_suffix_, 
        string res_suffix_, 
        string pow_suffix_)
{
    clustering_dir = clustering_dir_;
    clustering_res_dir = clustering_dir_+"/data/";
    cat_file_list_des_filename = clustering_res_dir+"/clustering.des";
    mid_suffix = mid_suffix_;
    res_suffix = res_suffix_;
    pow_suffix = pow_suffix_;
    clustering_file_status = is_rebuild;
    cout<<"ClusteringListDes init"<<endl;
    if(is_rebuild == TRUNCATED)
    {
        fs::remove_all(clustering_res_dir);
    }
    fs::create_directories(clustering_res_dir);
    load_cats();
    cout<<"ClusteringListDes init over"<<endl;
    return true;
}

}//namespace type
}//namespace clustering
}
}

