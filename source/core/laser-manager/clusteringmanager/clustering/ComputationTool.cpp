#include "ComputationTool.h"


namespace sf1r { namespace laser { namespace clustering {
void ComputationTool::run()
{
    type::ClusteringInfo info;
    while(getNext(info))
    {
        izene_reader_pointer reader = openFile<izene_reader>(info.clusteringMidPath, true);
        izene_writer_pointer writer = openFile<izene_writer>(info.clusteringResPath, true);
        izene_writer_pointer clustering_term_pow_writer = openFile<izene_writer>(info.clusteringPowPath, true);
        bool next = true;
        type::ClusteringInfo clusteringInfo;
        type::ClusteringData clusteringData;
        std::vector<type::Document>& c_cat_vector = clusteringData.clusteringData;
        c_cat_vector.reserve(max_clustering_doc_num);
        boost::unordered_map<std::string, float>& mean_pow = clusteringInfo.clusteringPow;
        typedef std::pair<std::string, type::Document> ValueClass;
        typedef hash_t KeyClass;
        KeyClass lastKey = -1;
        ValueClass lastValue;
        do 
        {
            ValueClass value;
            KeyClass key = -1;
            next = reader->Next(key, value);
            if (key != lastKey)   //new clustering or the last document
            {
                if (c_cat_vector.size() > min_clustering_doc_num)
                {
                    int id = map(lastValue.first);
                    clusteringData.clusteringHash = id;
                    clusteringInfo.clusteringHash = id;
                    clusteringInfo.clusteringname = "";
                    for (std::vector<type::Document>::iterator iter =
                                    c_cat_vector.begin(); iter != c_cat_vector.end();
                                iter++)
                    {
                        writer->Append(lastValue.first, *iter);
                    }
                    int c_count = c_cat_vector.size();
                    for (boost::unordered_map<std::string, float>::iterator iter = mean_pow.begin();
                                iter != mean_pow.end(); iter++)
                    {
                        iter->second /= c_count;
                    }
                    clusteringInfo.clusteringDocNum = c_count;
                    //save to kv-db or else
                    clusteringDataAdpater->save(clusteringData, clusteringInfo);
                    clustering_term_pow_writer->Append(lastValue.first, mean_pow);
                }
                mean_pow.clear();
                c_cat_vector.clear();
                lastKey = key;
                lastValue.first = value.first;
            }
            type::Document new_value(value.second.doc_id);
            boost::unordered_map<std::string, float> tf = value.second.terms;
            for (boost::unordered_map<std::string, float>::iterator iter = tf.begin();
                        iter != tf.end(); iter++)
            {
                    //erase the term which is not in the dictionary
                boost::unordered_map<std::string, std::pair<int, int> >::const_iterator finder = terms_.find(iter->first);
                if (finder != terms_.end())
                {
                    new_value.add(finder->first, iter->second);
                }
            }
            if (new_value.terms.size() < 2)
            {
                    //cout<<"REMOVE DOC:"<<value.doc_id<<endl;
            }
            else
            {
                for (boost::unordered_map<std::string, float>::iterator iter =
                                new_value.terms.begin(); iter != new_value.terms.end();
                            iter++)
                {
                    mean_pow[iter->first] += iter->second;
                }
                c_cat_vector.push_back(new_value);
            }
        }
        while (next);
        
        {
            // code for last category
                if (c_cat_vector.size() > min_clustering_doc_num)
                {
                    int id = map(lastValue.first);
                    clusteringData.clusteringHash = id;
                    clusteringInfo.clusteringHash = id;
                    clusteringInfo.clusteringname = "";
                    for (std::vector<type::Document>::iterator iter =
                                    c_cat_vector.begin(); iter != c_cat_vector.end();
                                iter++)
                    {
                        writer->Append(lastValue.first, *iter);
                    }
                    int c_count = c_cat_vector.size();
                    for (boost::unordered_map<std::string, float>::iterator iter = mean_pow.begin();
                                iter != mean_pow.end(); iter++)
                    {
                        iter->second /= c_count;
                    }
                    clusteringInfo.clusteringDocNum = c_count;
                    //save to kv-db or else
                    clusteringDataAdpater->save(clusteringData, clusteringInfo);
                    clustering_term_pow_writer->Append(lastValue.first, mean_pow);
                }
        }
        closeFile<izene_writer>(clustering_term_pow_writer);
        closeFile<izene_reader>(reader);
        closeFile<izene_writer>(writer);
    }
}
} } }
