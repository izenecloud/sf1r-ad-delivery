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

namespace sf1r
{
namespace laser
{
namespace clustering
{
using namespace sf1r::laser::clustering::type;
using namespace std;
using namespace boost;
/**
 * dic: pca dictionary path
 * f: threhold for pca clustering
 * clusteringresult: clustering result path
 * cat_dic: clustering category dictionary
 */
PCAClustering::PCAClustering(string term_directory_dir, string clustering_root_path,
                             float threhold_, int min_doc, int max_doc, int max_term, int threadnum) :
    threhold(threhold), 
    term_dictionary(clustering_root_path),
    min_clustering_doc_num(min_doc), 
    max_clustering_doc_num(max_doc), 
    max_clustering_term_num(max_term),
    segmentTool_(threadnum, term_dictionary, term_directory_dir, threhold_, max_doc)
{
    if (!ClusteringListDes::get()->init(clustering_root_path)
            || !CatDictionary::get()->init(clustering_root_path))
    {
        exit(0);
    }
    segmentTool_.start();

}

PCAClustering::~PCAClustering()
{
    term_dictionary.save();
    ClusteringListDes::get()->close();
    CatDictionary::get()->close();
    segmentTool_.stop();
}

void PCAClustering::execute(ClusteringDataAdapter* cda, int threadnum)
{
    segmentTool_.stop();
    ClusteringListDes::get()->closeMidWriterFiles();
    //unordered_map<string, string> catlist = ClusteringListDes::get()->get_cat_list();
    //unordered_map<string, string> catpathlist = ClusteringListDes::get()->get_cat_path();
    //limit the term number
    term_dictionary.sort(max_clustering_term_num);
    unordered_map<string, pair<int, int> > terms = term_dictionary.getTerms();
    queue<string> paths;
    ClusteringListDes::get()->generate_clustering_mid_result_paths(paths);
    ClusteringSortTool st(threadnum, paths);
    //sort the file
    st.start();
    queue<ClusteringInfo> infos;
    ClusteringListDes::get()->get_clustering_infos(infos); 
    ComputationTool ct(threadnum, infos, min_clustering_doc_num, max_clustering_doc_num,terms, cda);
    ct.start();
}

void PCAClustering::next(string title, string category, string docid)
{
    OriDocument od(title, category, docid);
    doc_map[category].push_back(od);
    if(doc_map[category].size() > 1000)
    {
        cout<<"to push"<<endl;
        segmentTool_.push_back(category, doc_map[category]);
        doc_map[category].clear();
        cout<<"push over"<<endl;
    }
    return;
}
}
}
}
