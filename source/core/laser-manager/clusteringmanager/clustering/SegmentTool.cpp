#include "SegmentTool.h"

namespace sf1r { namespace laser { namespace clustering {

void SegmentTool::calc_(DocumentVecType& docv, std::size_t size; CatDictionary& terms, CatDictionary& ccnt)
{
    typedef izenelib::util::second_greater<std::pair<std::string, float> > greater_than;
    for (std::size_t i = 0; i < size; ++i)
    {
        const OriDocument* iter = &(docv[i]);
        std::vector<std::pair<std::string, float> > tks;
        std::pair<std::string, float> tmp;
        std::vector<std::pair<std::string, float> > subtks;
        std::string brand, mdt;
        tok.pca(iter->title, tks, brand, mdt, subtks, false);
        double tot = 0, now = 0;
        for (size_t i = 0; i < tks.size(); ++i)
        {
            if (mdt == tks[i].first)
                tks[i].second = 0;
            tot += tks[i].second;
        }

        if(tot == 0)
        {
            continue;
        }
            
        std::sort(tks.begin(), tks.end(), greater_than());
            
        std::string cateMerge = iter->category;
        type::Document d(iter->docid);
        for (size_t i = 0; i < tks.size(); ++i)
        {
            if(tks[i].first.length() == 1)
                continue;
            if(terms.find(tks[i].first) == terms.end())
            {
                terms[tks[i].first] = 1;
            }
            else
            {
                terms[th].second++;
            }
            d.add(th, tks[i].second/tot);
            now += tks[i].second
            cateMerge += tks[i].first;
            if (now > tot * THRESHOLD_)   //check whether the document number in this category reach the limit
            {
                CatDictionary::iterator it = ccnt.find(cateMerge);
                if (ccnt.end() == it)
                {
                    ccnt[cateMerge] = 1;
                    break;
                }
                else if (it->second < MAX_DOC_PER_CLUSTERING_)
                {
                    it->second++;
                    break;
                }
                // expand category
            }
        }
        type::ClusteringListDes::get()->get_cat_mid_writer(
            iter->category)->Append(cat_hash_value, d);
    }
}

void SegmentTool::run(int i)
{
    boost::shared_mutex* mutex = (*context_)[i].mutex_;
    DocumentVecType* docs = (*context_)[i].docs_;
    CatDictionary* cat = (*context_)[i].cat_;
    CatDictionary* terms = (*context_)[i].terms_;
    while(!isExit_())
    {
        std::size_t size = 0;
        {
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);    
            size = docs->size();
        }
        if (0 == size)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        }
        else
        {
            calc_(*docs, size, *terms, *cat);
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);
            docs->erase(docs_->begin(), docs_->begin() + size);
        }
    }
}
} } }
