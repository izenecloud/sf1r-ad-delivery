#include "PCARunner.h"

const std::size_t MAX_TOKEN = 4;
namespace sf1r { namespace laser { namespace clustering {

PCARunner::DocumentVecType::iterator  PCARunner::distribution_(DocumentVecType& docv, 
    const std::size_t size, 
    Dictionary& ccnt, 
    Dictionary& dict)
{
    DocumentVecType::iterator iter = docv.begin();
    for (std::size_t k = 0; k < size; ++k, ++iter)
    {
        std::vector<std::pair<std::string, float> > tks;
        tokenize_(iter->title, tks);
        if (tks.empty())
            continue;
        
        std::string cateMerge = iter->category;
        Dictionary::iterator it = ccnt.find(cateMerge);
        if (ccnt.end() == it)
        {
            ccnt[cateMerge] = 1;
        }
        else
        {
            it->second++;
        }
        
        for (std::size_t i = 0; i < tks.size(); ++i)
        {
            if(dict.find(tks[i].first) == dict.end())
            {
                dict[tks[i].first] = 1;
            }
            else
            {
                dict[tks[i].first]++;
            }
            
            cateMerge += tks[i].first;
            Dictionary::iterator it = ccnt.find(cateMerge);
            if (ccnt.end() == it)
            {
                ccnt[cateMerge] = 1;
            }
            else
            {
                it->second++;
            }
            if (i > MAX_TOKEN)
            {
                break;
            }
        }
    }
    return iter;
}

void PCARunner::statistics_(ThreadContext* context)
{
    boost::shared_mutex* mutex = context->mutex_;
    DocumentVecType* docs = context->docs_;
    Dictionary* cat = context->cat_;
    Dictionary* terms = context->terms_;
    while(true)
    {
        std::size_t size = 0;
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);    
            size = docs->size();
        }
        if (0 == size)
        {
            if (isExit_())
                break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
        else
        {
           DocumentVecType::iterator end = distribution_(*docs, size, *cat, *terms);
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);
            docs->erase(docs->begin(), end);
        }
    }
}

PCARunner::DocumentVecType::iterator PCARunner::doClustering_(DocumentVecType& docv, 
    const std::size_t size, 
    const Dictionary& ccnt, 
    const type::TermDictionary& dict, 
    ClusteringContainer& clusteringContainer)
{
    DocumentVecType::iterator iter = docv.begin();
    for (std::size_t k = 0; k < size; ++k, ++iter)
    {
        std::vector<std::pair<std::string, float> > tks;
        tokenize_(iter->title, tks);
        if (tks.empty())
            continue;
        
        const std::size_t size = tks.size();
        std::string cateMerge = iter->category;
        {
            Dictionary::const_iterator it = ccnt.find(cateMerge);
            if (it->second < MIN_DOC_PER_CLUSTERING_)
            {
                continue;
            }
            if (it->second < MAX_DOC_PER_CLUSTERING_)
            {
                pushClustering_(cateMerge, tks, dict, clusteringContainer); 
                continue;
            }
        }

        for (std::size_t i = 0; i < size; ++i)
        {
            cateMerge += tks[i].first;
            {
                Dictionary::const_iterator it = ccnt.find(cateMerge);
                if (ccnt.end() == it)
                {
                    continue;
                }
                else if (it->second > MAX_DOC_PER_CLUSTERING_)
                {
                    if ( i + 1 < size) 
                    {
                        Dictionary::const_iterator next = ccnt.find(cateMerge + tks[i + 1].first);
                        if (next == ccnt.end())
                        {
                            break;
                        }

                        if (next->second < MIN_DOC_PER_CLUSTERING_)
                        {
                            break;
                        }

                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        
       pushClustering_(cateMerge, tks, dict, clusteringContainer); 
    }
    return iter;
}

void PCARunner::clustering_(ThreadContext* context)
{
    boost::shared_mutex* mutex = context->mutex_;
    DocumentVecType* docs = context->docs_;
    const Dictionary* cat = context->cat_;
    const type::TermDictionary& dict = term_dictionary;
    ClusteringContainer* clusteringContainer = context->clusteringContainer_;
    while(true)
    {
        std::size_t size = 0;
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);    
            size = docs->size();
        }
        if (0 == size)
        {
            if (isExit_())
                break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        }
        else
        {
            DocumentVecType::iterator end = doClustering_(*docs, size, *cat, dict, *clusteringContainer);
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);
            docs->erase(docs->begin(), end);
        }
    }
}

void PCARunner::tokenize_(const std::string title, std::vector<std::pair<std::string, float> >& tks)
{
    typedef izenelib::util::second_greater<std::pair<std::string, float> > greater_than;
    std::vector<std::pair<std::string, float> > subtks;
    std::string brand, mdt;
    tok.pca(title, tks, brand, mdt, subtks, false);
    double tot = 0;
    std::vector<std::pair<std::string, float> >::iterator it = tks.begin();
    for (; tks.end() != it; )
    {
        if (it->first.size() == 1)
        {
            it = tks.erase(it);
            continue;
        }
        if (mdt == it->first)
        {
            it->second = 0;
        }
        else
        {
            tot += it->second;
        }
        ++it;
    }

    if(tot == 0)
    {
        tks.clear();
    }
            
    std::sort(tks.begin(), tks.end(), greater_than());
    
    for (std::size_t i = 0; i < tks.size(); ++i)
    {
        tks[i].second /= tot;
    }
}

void PCARunner::pushClustering_(const std::string& cateMerge, 
    const std::vector<std::pair<std::string, float> >& tks,
    const type::TermDictionary& dict,
    ClusteringContainer& clusteringContainer) const
{
    //LOG(INFO)<<"pushClustering_ "<<cateMerge;
    std::size_t size = tks.size();
    ClusteringContainer::iterator it = clusteringContainer.find(cateMerge);
    if (clusteringContainer.end() == it)
    {
        Clustering clustering;
        clustering.first = 1;
        for (std::size_t i = 0; i < size; ++i)
        {
            // remove tokens not in dictionary
            if (dict.find(tks[i].first))
            {
                clustering.second[tks[i].first] = tks[i].second;
            }
        }
        clusteringContainer[cateMerge] = clustering;
    }
    else
    {
        Clustering& clustering = it->second;
        (clustering.first)++;
        for (std::size_t i = 0; i < size; ++i)
        {
            // remove tokens not in dictionary
            if (dict.find(tks[i].first))
            {
                if (clustering.second.end() != clustering.second.find(tks[i].first))
                {
                    (clustering.second)[tks[i].first] += tks[i].second; 
                }
                else
                {
                    (clustering.second)[tks[i].first] = tks[i].second; 
                }
            }
        }
    }
}

void saveClusteringResult(const std::vector<boost::unordered_map<std::string, float> >& container, const std::string& filename)
{
    std::ofstream ofs(filename.c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << container;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}

void loadClusteringResult(std::vector<boost::unordered_map<std::string, float> >& container, const std::string& filename)
{
    std::ifstream ifs(filename.c_str(), std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
    ia >> container;
}

} } }
