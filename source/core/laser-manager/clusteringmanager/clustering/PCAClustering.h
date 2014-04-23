/*
 * Pcaclustering.h
 *
 *  Created on: Mar 27, 2014
 *      Author: alex
 */

#ifndef PCACLUSTERING_H_
#define PCACLUSTERING_H_
#include <string>
#include "knlp/title_pca.h"
#include "am/sequence_file/ssfr.h"
#include <map>
#include "util/hashFunction.h"
#include "type/CatDictionary.h"
#include "type/Term.h"
#include "type/ClusteringListDes.h"
#include "type/TermDictionary.h"
#include "clustering/ComputationTool.h"
#include "clustering/SortTool.h"
#include "type/ClusteringDataAdapter.h"
using namespace ilplib::knlp;
using namespace izenelib;
using namespace clustering::type;
namespace clustering
{
class PCAClustering
{
private:
    float threhold;//the clustering threhold
    TitlePCA tok; //the pca
    //cat_dic will maintain the final clustering list
    //CatDictionary cat_dic_;
    //term_dictionary will maintain the term dictionary, sort and reduce the dimention
    TermDictionary term_dictionary;
    //clusteringListDes will descript the first partition according to orignal category
    //ClusteringListDes clusteringListDes;
    //min document number in a certain clustering
    int min_clustering_doc_num;
    //max document number in a certain clustering
    int max_clustering_doc_num;
    //max term number in the clustering
    int max_clustering_term_num;
public:
    bool loadDic(const std::string& dic_path);
    PCAClustering(string term_directory_dir, string clustering_root_path, float threhold_= 0.1,  int min_doc=10, int max_doc=1000, int max_term=10000) ;
    void next(std::string title, std::string category, std::string docid);
    void execute(ClusteringDataAdapter * cda, int threadnum);
    ~PCAClustering();
    bool init();
};
}
#endif /* PCACLUSTERING_H_ */
