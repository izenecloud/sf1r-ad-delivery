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
#include "PCAClustering.h"
#include "laser-manager/clusteringmanager/type/ClusteringListDes.h"
#include "laser-manager/clusteringmanager/common/utils.h"
#include "laser-manager/clusteringmanager/type/Document.h"
namespace fs = boost::filesystem;
using std::string;
using namespace sf1r::laser::clustering::type;

namespace sf1r
{
namespace laser
{
namespace clustering
{
/**
 * dic: pca dictionary path
 * f: threhold for pca clustering
 * clusteringresult: clustering result path
 * cat_dic: clustering category dictionary
 */
PCAClustering::PCAClustering(string term_directory_dir, string clustering_root_path,
                             float threhold_, int min_doc, int max_doc, int max_term, int threadnum) :
    threhold(threhold), 
    term_dictionary(clustering_root_path, TRUNCATED),
    min_clustering_doc_num(min_doc), 
    max_clustering_doc_num(max_doc), 
    max_clustering_term_num(max_term),
    segmentTool_(threadnum, term_dictionary, term_directory_dir, threhold_, max_doc)
{
    if (!ClusteringListDes::get()->init(clustering_root_path, TRUNCATED)
            || !CatDictionary::get()->init(clustering_root_path, TRUNCATED))
    {
        exit(0);
    }
    segmentTool_.start();

}

PCAClustering::~PCAClustering()
{
    ClusteringListDes::get()->close();
    CatDictionary::get()->close();
    segmentTool_.stop();
}

void PCAClustering::execute(ClusteringDataAdapter* cda, int threadnum)
{
    segmentTool_.stop();
    ClusteringListDes::get()->closeMidWriterFiles();
    std::map<hash_t, string> catlist = ClusteringListDes::get()->get_cat_list();
    std::map<hash_t, string> catpathlist = ClusteringListDes::get()->get_cat_path();
    //limit the term number
    term_dictionary.sort(max_clustering_term_num);
    map<hash_t, Term> terms = term_dictionary.getTerms();
    std::queue<string> paths= ClusteringListDes::get()->generate_clustering_mid_result_paths();
    ClusteringSortTool st(threadnum, paths);
    //sort the file
    st.start();
    ComputationTool ct(threadnum,ClusteringListDes::get()->get_clustering_infos(), min_clustering_doc_num, max_clustering_doc_num,terms, cda);
    ct.start();
}

void PCAClustering::next(string title, string category, string docid)
{
    OriDocument od(title, category, docid);
    doc_map[category].push_back(od);
    if(doc_map[category].size() > 1000)
    {
        cout<<"to push"<<endl;
        segmentTool_.pushback(category, doc_map[category]);
        doc_map[category].clear();
        cout<<"push over"<<endl;
    }
/*    for(map<string, SegmentTool::DocumentVecType>::iterator iter = doc_map.begin(); iter != doc_map.end(); iter++)
    {
        if(iter->second.size() >0)
        {
            segmentTool_.pushback(iter->first, iter->second);
        }
    }*/
    return;
}
}
}
}
