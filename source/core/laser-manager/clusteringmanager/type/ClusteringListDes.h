/*
 * CatFiles.h
 *
 *  Created on: Mar 31, 2014
 *      Author: alex
 */

#ifndef CATFILES_H_
#define CATFILES_H_
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <map>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "common/constant.h"
#include "common/utils.h"
#include <util/singleton.h>
#include "type/ClusteringInfo.h"

namespace fs = boost::filesystem;

using std::fstream;
using std::string;
using std::map;

namespace clustering
{
namespace type
{

/**
 *@Description CatFiles maintains the middle result for
 */
class ClusteringListDes
{
private:
    //we split all the item with their orignal category, it aims to reduce the sort time and supply the possiblity to parrel merge
    //clustering_dir: represents the clustering root dir
    string clustering_dir;
    //clustering_res_dir: represents the clustering result dir
    string clustering_res_dir;
    string cat_file_list_des_filename;
    string mid_suffix;
    string res_suffix;
    string pow_suffix;
    fstream cat_file_list_des;
    /**
     * mid result writer
     */
    map<hash_t, izene_writer_pointer> cat_mid_clustering_writers_;
    /**
     * mid result writer
     */
    map<hash_t, izene_reader_pointer> cat_mid_clustering_readers_;

    /**
     * final result writer
     */
    map<hash_t, izene_writer_pointer> cat_final_clustering_writers_;
    /**
     * final result writer
     */
    map<hash_t, izene_reader_pointer> cat_final_clustering_readers_;

    map<hash_t, string> cat_name_map;
    /**
     * map from orignal cat hash to mid clustering result path
     */
    map<hash_t, string> cat_path_map;
    /**
     * rebuild or append
     */
    STATUS clustering_file_status;
    template<class izenestream>
    izenestream* init_stream(hash_t hnum, string path,
                             map<hash_t, izenestream*>& clustering, bool must)
    {
        typename map<hash_t, izenestream*>::iterator iter = clustering.find(hnum);
        izenestream* izene_ptr = NULL;
        if (iter == clustering.end())
        {
            izene_ptr = openFile<izenestream>(path.c_str(), must);
            clustering.insert(
                std::make_pair<hash_t, izenestream*>(hnum, izene_ptr));
        }
        else
        {
            izene_ptr = iter->second;
        }
        return izene_ptr;
    }
    inline izene_writer_pointer get_mid_cat_writer(hash_t hnum)
    {
        string cat_mid_res_path = generate_clustering_mid_result_path(hnum);
        return init_stream<izene_writer>(hnum, cat_mid_res_path,
                                         cat_mid_clustering_writers_, true);
    }

    inline izene_reader_pointer get_mid_cat_reader(hash_t hnum)
    {
        string cat_mid_path = generate_clustering_mid_result_path(hnum);
        return init_stream<izene_reader>(hnum, cat_mid_path,
                                         cat_mid_clustering_readers_, false);
    }

    inline izene_writer_pointer get_final_cat_writer(hash_t hnum)
    {
        string cat_final_res_path = generate_clustering_final_result_path(hnum);
        return init_stream<izene_writer>(hnum, cat_final_res_path,
                                         cat_final_clustering_writers_, true);
    }

    inline izene_reader_pointer get_final_cat_reader(hash_t hnum)
    {
        string cat_final_res_path = generate_clustering_final_result_path(hnum);
        return init_stream<izene_reader>(hnum, cat_final_res_path,
                                         cat_final_clustering_readers_, false);
    }
    void save_cats()
    {
        cat_file_list_des.open(cat_file_list_des_filename.c_str(), ios::out);
        for(map<hash_t, string>::iterator iter = cat_name_map.begin(); iter != cat_name_map.end(); iter++)
        {
            cat_file_list_des<<iter->first<<"\t"<<iter->second<<endl;
        }
        cat_file_list_des.close();
    }
    void load_cats()
    {
        string line;
        cat_file_list_des.open(cat_file_list_des_filename.c_str(), ios::in);
        //cat_file_list_des.seekg(0, std::fstream::beg);
        while (getline(cat_file_list_des, line))
        {
            int index = line.find("\t");
            if (index == -1)
                continue;
            hash_t cat_h = std::strtoul(line.substr(0, index).c_str(), NULL,
                                        10);
            //init_cat_writer(cat_h);
            generate_clustering_mid_result_path(cat_h);
            //cat_midpath_map.insert(make_pair<hash_t, string> (cat_h,cat_path));
            cat_name_map.insert(
                std::make_pair<hash_t, string>(cat_h,
                                               line.substr(index + 1)));
        }
        cat_file_list_des.close();
    }

public:
    ClusteringListDes()
    {
    }
    /**
     * clustering_dir: represents the clustering root dir
     * mid_suffix_: suffix for mid clustering result path
     */

    bool init(string clustering_dir_, STATUS is_rebuild = ONLY_READ,
              string mid_suffix_ = ".mid", string res_suffix_ = ".res",
              string pow_suffix_ = ".pow");

    //singleton module
    inline static ClusteringListDes* get()
    {
        return izenelib::util::Singleton<ClusteringListDes>::get();
    }

    void close()
    {
        save_cats();
        closeFiles<izene_writer>(cat_mid_clustering_writers_);
        closeFiles<izene_reader>(cat_mid_clustering_readers_);
        closeFiles<izene_writer>(cat_final_clustering_writers_);
        closeFiles<izene_reader>(cat_final_clustering_readers_);
        cout<<"cat_file_list_des close"<<endl;

        cat_mid_clustering_writers_.clear();
        cat_mid_clustering_readers_.clear();
        cat_final_clustering_writers_.clear();
        cat_final_clustering_readers_.clear();
    }

    ~ClusteringListDes()
    {

    }

    void closeMidWriterFiles()
    {
        closeFiles<izene_writer>(cat_mid_clustering_writers_);
        cat_mid_clustering_writers_.clear();
    }

    queue<ClusteringInfo> get_clustering_infos()
    {
        std::queue<ClusteringInfo> infos;
        for (map<hash_t, string>::iterator iter = cat_path_map.begin();
                iter != cat_path_map.end(); iter++)
        {
            map<hash_t, string>::iterator finder = cat_name_map.find(
                    iter->first);
            if (finder == cat_name_map.end())
                continue;
            ClusteringInfo ci;
            ci.clusteringname = finder->second;
            ci.clusteringMidPath = generate_clustering_mid_result_path(finder->first);
            ci.clusteringResPath = generate_clustering_final_result_path(
                                       finder->first);
            ci.clusteringPowPath = generate_clustering_pow_result_path(finder->first);
            infos.push(ci);
        }
        return infos;
    }

    queue<string> generate_clustering_mid_result_paths()
    {
        std::queue<string> paths;
        for (map<hash_t, string>::iterator iter = cat_path_map.begin();
                iter != cat_path_map.end(); iter++)
        {
            paths.push(iter->second + mid_suffix);
        }
        return paths;
    }

    string generate_path(hash_t hnum, const string& suffix)
    {
        std::stringstream cat_path;
        cat_path << clustering_res_dir << "/" << hnum;
        cat_path_map.insert(make_pair<hash_t, string>(hnum, cat_path.str()));
        cat_path << suffix;
        return cat_path.str();
    }

    string generate_clustering_pow_result_path(hash_t hnum)
    {
        return generate_path(hnum, pow_suffix);
    }

    string generate_clustering_mid_result_path(hash_t hnum)
    {
        return generate_path(hnum, mid_suffix);
    }
    /**
     * generate the clustering final result path
     */
    string generate_clustering_final_result_path(hash_t hnum)
    {
        return generate_path(hnum, res_suffix);
    }

    izene_writer_pointer get_cat_mid_writer(string cat)
    {
        hash_t ht = Hash_(cat);
        if (cat_name_map.find(ht) == cat_name_map.end())
        {
            cat_file_list_des << ht << "\t" << cat << std::endl;
            cat_name_map.insert(std::make_pair<hash_t, string>(ht, cat));
        }
        return get_mid_cat_writer(ht);
    }

    std::map<hash_t, string> get_cat_list()
    {
        return this->cat_name_map;
    }
    std::map<hash_t, string> get_cat_path()
    {
        return this->cat_path_map;
    }

    string getCatFileListDesFilename()
    {
        return cat_file_list_des_filename;
    }

    void setCatFileListDesFilename(string catFileListDesFilename)
    {
        cat_file_list_des_filename = catFileListDesFilename;
    }

    map<hash_t, izene_reader_pointer> getCatFinalClusteringReaders()
    {
        return cat_final_clustering_readers_;
    }

    void setCatFinalClusteringReaders(
        map<hash_t, izene_reader_pointer> catFinalClusteringReaders)
    {
        cat_final_clustering_readers_ = catFinalClusteringReaders;
    }

    map<hash_t, izene_writer_pointer> getCatFinalClusteringWriters()
    {
        return cat_final_clustering_writers_;
    }

    void setCatFinalClusteringWriters(
        map<hash_t, izene_writer_pointer> catFinalClusteringWriters)
    {
        cat_final_clustering_writers_ = catFinalClusteringWriters;
    }

    map<hash_t, izene_reader_pointer> getCatMidClusteringReaders()
    {
        return cat_mid_clustering_readers_;
    }

    void setCatMidClusteringReaders(
        const map<hash_t, izene_reader_pointer>& catMidClusteringReaders)
    {
        cat_mid_clustering_readers_ = catMidClusteringReaders;
    }

    map<hash_t, izene_writer_pointer>& getCatMidClusteringWriters()
    {
        return cat_mid_clustering_writers_;
    }

    const map<hash_t, string>& getCatNameMap() const
    {
        return cat_name_map;
    }

    const map<hash_t, string>& getCatPathMap() const
    {
        return cat_path_map;
    }

    const string& getClusteringDir() const
    {
        return clustering_dir;
    }

    STATUS getClusteringFileStatus()
    {
        return clustering_file_status;
    }

    void setClusteringFileStatus(STATUS clusteringFileStatus)
    {
        clustering_file_status = clusteringFileStatus;
    }

    string getClusteringResDir()
    {
        return clustering_res_dir;
    }

    void setClusteringResDir(string clusteringResDir)
    {
        clustering_res_dir = clusteringResDir;
    }

    string getMidSuffix()
    {
        return mid_suffix;
    }

    void setMidSuffix(string midSuffix)
    {
        mid_suffix = midSuffix;
    }

    string getResSuffix()
    {
        return res_suffix;
    }

    void setResSuffix(const string& resSuffix)
    {
        res_suffix = resSuffix;
    }
};
}
}
#endif /* CATFILES_H_ */
