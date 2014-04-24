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
                             float threhold_, int min_doc, int max_doc, int max_term) :
    threhold(threhold_), tok(term_directory_dir), term_dictionary(
        clustering_root_path, TRUNCATED), min_clustering_doc_num(min_doc), max_clustering_doc_num(
            max_doc), max_clustering_term_num(max_term)
{
    if (!ClusteringListDes::get()->init(clustering_root_path, TRUNCATED)
            || !CatDictionary::get()->init(clustering_root_path, TRUNCATED))
    {
        exit(0);
    }

}

PCAClustering::~PCAClustering()
{
    ClusteringListDes::get()->close();
    CatDictionary::get()->close();

}

void PCAClustering::execute(ClusteringDataAdapter* cda, int threadnum)
{
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
    std::vector<std::pair<std::string, float> > tks;
    std::pair<std::string, float> tmp;
    std::vector<std::pair<std::string, float> > subtks;
    std::string brand, mdt;
    izene_writer_pointer iwp = ClusteringListDes::get()->get_cat_mid_writer(
                                   category);
    tok.pca(title, tks, brand, mdt, subtks, false);
    double tot = 0, now = 0;
    for (size_t i = 0; i < tks.size(); ++i)
    {
        if (mdt == tks[i].first)
            tks[i].second = 0;
        tot += tks[i].second;
    }
    for (size_t i = 0; i < tks.size(); ++i)
    {
        for (size_t j = i + 1; j < tks.size(); ++j)
        {
            if (tks[i].second < tks[j].second)
            {
                tmp = tks[i];
                tks[i] = tks[j];
                tks[j] = tmp;
            }
        }
    }
    std::stringstream category_merge;
    category_merge << category;
    Document d(docid);
    size_t cat_limit_count = std::numeric_limits<size_t>::max();
    hash_t cat_hash_value = 0;
    for (size_t i = 0; i < tks.size(); ++i)
    {
        hash_t tid = term_dictionary.get(tks[i].first);
        //give up the term
        if (tid == 0)
            continue;
        d.add(tid, tks[i].second);

        now += tks[i].second;
        if (now > tot * threhold)   //check whether the document number in this category reach the limit
        {
            if (cat_limit_count > max_clustering_doc_num)   // temporary number{
            {
                category_merge << "<" << tks[i].first;
                pair<hash_t, size_t> hs = CatDictionary::get()->getCatHash(
                                              category_merge.str());
                cat_limit_count = hs.second;
                if (cat_limit_count < max_clustering_doc_num)
                {
                    //cout<<"limit count is"<<cat_limit_count<<endl;
                    CatDictionary::get()->addCatHash(category_merge.str(),
                                                     hs.first);
                    cat_hash_value = hs.first;
                }
                else
                {
                    //category_merge<<"<"<<tks[i].first;
                }
            }
        }
        else
        {
            category_merge << "<" << tks[i].first;
            now += tks[i].second;
        }

    }
    //hash_t h = CatDictionary::get()->getCatHash(category_merge.str());
    if (cat_hash_value > 0)
        iwp->Append(cat_hash_value, d);

}
}
}
}
