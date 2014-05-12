#include "PredictTool.h"
#include "laser-manager/clusteringmanager/type/ClusteringDataStorage.h"
#include "laser-manager/clusteringmanager/common/utils.h"
using namespace sf1r::laser::clustering::type;
using namespace sf1r::laser::clustering;

namespace sf1r { namespace laser { namespace predict {
void getTopN(const ClusterContainer& clusters,const PerUserOnlineModel& puseronlineModel, size_t topN, MinHeapDocRankType& minHeap)
{
        ClusterContainerIter iter = clusters.begin();
        cout<<puseronlineModel.getUserName()<<endl;
        const vector<float>& args = puseronlineModel.getArgs();
//        for(size_t i = 0; i < args.size(); i++)
//        {
//            cout<<args[i]<<" ";
//        }
//        cout<<endl;

        for(; iter != clusters.end(); iter++)
        {
            clustering::type::ClusteringData cd;
            //cout<<"To find "<<iter->first<<endl;
            ClusteringDataStorage::get()->loadClusteringData(iter->first, cd);
            for(std::vector<Document>::iterator doc_iter = cd.clusteringData.begin(); doc_iter != cd.clusteringData.end(); doc_iter++)
            {
                Document::TermMapType::const_iterator term_iter = doc_iter->terms.begin();
                float docrank = 0;
                for(; term_iter != doc_iter->terms.end(); term_iter++)
                {
              //      cout<<"iter:"<<term_iter->first<<"term feature"<<term_iter->second<<" args:"<<args[term_iter->first]<<endl;
                    // 0th is delta
                    docrank +=term_iter->second*args[Hash_(term_iter->first) + 1];
                }
                docrank += args[0];
                if(minHeap.size() >= topN)
                {
                    if(docrank > minHeap.top().rank_)
                    {
                        DocRank dr(doc_iter->doc_id, docrank);
                        minHeap.pop();
                        minHeap.push(dr);
                    }
                }
                else
                {
                    DocRank dr(doc_iter->doc_id, docrank);
                    minHeap.push(dr);
                }

            }
        }
    }
} } }
