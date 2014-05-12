#include "SegmentTool.h"
#include "laser-manager/clusteringmanager/type/Document.h"
#include "laser-manager/clusteringmanager/type/ClusteringListDes.h"

namespace sf1r { namespace laser { namespace clustering {

SegmentTool::DocumentVecType::iterator  SegmentTool::calc_(DocumentVecType& docv, std::size_t size, Dictionary& ccnt, Dictionary& termList)
{
    typedef izenelib::util::second_greater<std::pair<std::string, float> > greater_than;
    DocumentVecType::iterator iter = docv.begin();
    for (std::size_t i = 0; i < size; ++i, ++iter)
    {
        //const OriDocument* iter = &(docv[i]);
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
            if(termList.find(tks[i].first) == termList.end())
            {
                termList[tks[i].first] = 1;
            }
            else
            {
                termList[tks[i].first]++;
            }
        // TODO
        //}
        //for (size_t i = 0; i < tks.size(); ++i)
        //{
            d.add(tks[i].first, tks[i].second/tot);
            now += tks[i].second;
            cateMerge += tks[i].first;
            if (now > tot * THRESHOLD_)   //check whether the document number in this category reach the limit
            {
                Dictionary::iterator it = ccnt.find(cateMerge);
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
        //std::cout<<iter->category<<"\n";
        type::ClusteringListDes::get()->get_cat_mid_writer(
            iter->category)->Append(Hash_(cateMerge), std::make_pair(cateMerge, d));
    }
    return iter;
}

void SegmentTool::run(ThreadContext* context)
{
    boost::shared_mutex* mutex = context->mutex_;
    DocumentVecType* docs = context->docs_;
    Dictionary* cat = context->cat_;
    Dictionary* terms = context->terms_;
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
            DocumentVecType::iterator end = calc_(*docs, size, *cat, *terms);
            boost::unique_lock<boost::shared_mutex> uniqueLock(*mutex);
            docs->erase(docs->begin(), end);
        }
    }
}
} } }
