/*
 * CatFiles.h
 *
 *  Created on: Mar 31, 2014
 *      Author: alex
 */

#ifndef SF1R_LASER_CATFILES_H_
#define SF1R_LASER_CATFILES_H_
#include <fstream>
#include <string>
#include <sstream>
#include <queue>
#include <map>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <util/singleton.h>
#include "laser-manager/clusteringmanager/common/constant.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include "ClusteringInfo.h"

namespace sf1r { namespace laser { namespace clustering { namespace type {
/**
 *@Description CatFiles maintains the middle result for
 */
class ClusteringListDes
{
    
    inline izene_writer_pointer get_mid_cat_writer(const std::string& cate)
    {
        std::string cat_mid_res_path = generate_clustering_mid_result_path(cate);
        izenestream* writer = find_stream<izene_writer>(cate, cat_mid_clustering_writers_);
        if (NULL != writer)
            return writer;
        return init_stream<izene_writer>(cate, cat_mid_res_path,
                                         cat_mid_clustering_writers_, true);
    }

    inline izene_reader_pointer get_mid_cat_reader(const std::string& cate)
    {
        std::string cat_mid_path = generate_clustering_mid_result_path(cate);
        izenestream* reader = find_stream<izene_reader>(cate, cat_mid_clustering_readers_);
        if (NULL != reader)
            return reader;
        return init_stream<izene_reader>(cate, cat_mid_path,
                                         cat_mid_clustering_readers_, false);
    }

    inline izene_writer_pointer get_final_cat_writer(const std::string& cate)
    {
        std::string cat_final_res_path = generate_clustering_final_result_path(cate);
        izenestream* writer = find_stream<izene_writer>(cate, cat_final_clustering_writers_);
        if (NULL != writer)
            return writer;
        return init_stream<izene_writer>(cate, cat_final_res_path,
                                         cat_final_clustering_writers_, true);
    }

    inline izene_reader_pointer get_final_cat_reader(const std::string& cate)
    {
        std::string cat_final_res_path = generate_clustering_final_result_path(cate);
        izenestream* reader = find_stream<izene_reader>(cate, cat_final_clustering_readers_);
        if (NULL != reader)
            return reader;
        return init_stream<izene_reader>(cate, cat_final_res_path,
                                         cat_final_clustering_readers_, false);
    }

public:
    /**
     * clustering_dir: represents the clustering root dir
     * mid_suffix_: suffix for mid clustering result path
     */

    bool init(std::string clustering_dir_);

    //singleton module
    inline static ClusteringListDes* get()
    {
        return izenelib::util::Singleton<ClusteringListDes>::get();
    }

    void close()
    {
        closeFiles<izene_writer>(cat_mid_clustering_writers_);
        closeFiles<izene_reader>(cat_mid_clustering_readers_);
        closeFiles<izene_writer>(cat_final_clustering_writers_);
        closeFiles<izene_reader>(cat_final_clustering_readers_);

        cat_mid_clustering_writers_.clear();
        cat_mid_clustering_readers_.clear();
        cat_final_clustering_writers_.clear();
        cat_final_clustering_readers_.clear();
    }


    void closeMidWriterFiles()
    {
        closeFiles<izene_writer>(cat_mid_clustering_writers_);
        cat_mid_clustering_writers_.clear();
    }

    void get_clustering_infos(std::queue<ClusteringInfo>& infos) const
    {
        for (boost::unordered_map<std::string, std::string>::iterator iter = cat_path_map.begin();
                iter != cat_path_map.end(); iter++)
        {
            ClusteringInfo ci;
            ci.clusteringname = iter->first;
            ci.clusteringMidPath = generate_clustering_mid_result_path(iter->first);
            ci.clusteringResPath = generate_clustering_final_result_path(
                                       iter->first);
            ci.clusteringPowPath = generate_clustering_pow_result_path(it->first);
            infos.push(ci);
        }
    }

    /*queue<string> generate_clustering_mid_result_paths()
    {
        std::queue<string> paths;
        for (map<hash_t, string>::iterator iter = cat_path_map.begin();
                iter != cat_path_map.end(); iter++)
        {
            paths.push(iter->second + mid_suffix);
        }
        return paths;
    }*/

private:
    const std::string generate_path(const std::string& cate, const string& suffix)
    {
        std::string cat_path = workdir_ + cate + suffix;
        cat_path_map[cat_path] = cat_path;
        return cat_path;
    }

    const std::string generate_clustering_pow_result_path(const std::string& cate)
    {
        return generate_path(cate, pow_suffix);
    }

    const std::string generate_clustering_mid_result_path(const std::string& cate)
    {
        return generate_path(cate, mid_suffix);
    }
    /**
     * generate the clustering final result path
     */
    const std::string generate_clustering_final_result_path(const std::string& cate)
    {
        return generate_path(cate, res_suffix);
    }

    izene_writer_pointer get_cat_mid_writer(string cat)
    {
        return get_mid_cat_writer(ht);
    }
    
    template<class izenestream>
    izenestream* init_stream(const std::string cate, std::string path,
                             boost::unordered_map<std::string, izenestream*>& clustering, bool must)
    {
        boost::unique_lock<boost::shared_mutex> uniqueLock(mutex_);
        izenestream* izene_ptr = NULL;
        izene_ptr = openFile<izenestream>(path.c_str(), must);
        clustering[cate] = izene_ptr;
        return izene_ptr;
    }
    
    template<class izenestream>
    izenestream* find_stream(const std::string cate,
        boost::unordered_map<std::string, izenestream*>& clustering)
    {
        boost::shared_lock<boost::shared_mutex> sharedLock(mutex_);

        boost::unordered_map<std::string, izenestream*>::iterator iter = clustering.find(cate);
        if (iter == clustering.end())
        {
            return NULL;
        }
        else
        {
            return iter->second;
        }
    }

private:
    //we split all the item with their orignal category, it aims to reduce the sort time and supply the possiblity to parrel merge
    //clustering_dir: represents the clustering root dir
    std::string workdir_;
    /**
     * mid result writer
     */
    boost::unordered_map<std::string, izene_writer_pointer> cat_mid_clustering_writers_;
    /**
     * mid result writer
     */
    boost::unordered_map<std::string, izene_reader_pointer> cat_mid_clustering_readers_;

    /**
     * final result writer
     */
    boost::unordered_map<std::string, izene_writer_pointer> cat_final_clustering_writers_;
    /**
     * final result writer
     */
    boost::unordered_map<std::string, izene_reader_pointer> cat_final_clustering_readers_;

    /**
     * map from orignal cat hash to mid clustering result path
     */
    boost::unordered_map<std::string, std::string> cat_path_map;

    boost::shared_mutex mutex_;
    
    static const std::string MID_SUFFIX;
    static const std::string RES_SUFFIX;
    static const std::string POW_SUFFIX;
};

} } } }
#endif /* CATFILES_H_ */
    
    /*
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
    }*/
