#include <unistd.h>

#include <laser-manager/Tokenizer.h>
#include <vector>
#include <queue>
#include <boost/unordered_map.hpp>
#include <util/functional.h>
#include <laser-manager/clusteringmanager/clustering/PCARunner.h>

using namespace sf1r;
using namespace sf1r::laser;

const std::size_t N_SIM = 100;

typedef izenelib::util::second_greater<std::pair<int, float> > greater_than;
typedef std::priority_queue<std::pair<int, float>, std::vector<std::pair<int, float> >, greater_than> pri_queue;

typedef std::vector<float> TokenVector;

float similarity(const TokenVector& lv, const TokenVector& rv)
{
    if (lv.size() != rv.size())
    {
        return -1;
    }
    float sim = 0.0;
    float lsum = 0.0;
    float rsum = 0.0;
    for (std::size_t i = 0; i < lv.size(); ++i)
    {
        sim += lv[i] * rv[i];
        lsum += lv[i];
        rsum += rv[i];
    }
    return sim / lsum/ rsum;
}

int main(int argc, char * argv[])
{
    if (argc != 4)
    {
        return -1;
    }

    const std::string clustering = argv[1];
    const std::string termDict = argv[2];
    const std::string similarFilename = argv[3];

    std::vector<boost::unordered_map<std::string, float> > temp;
    laser::clustering::loadClusteringResult(temp, clustering);
    
    Tokenizer* tokenizer = new Tokenizer(termDict);    
   
    std::vector<TokenVector>* clusteringContainer = new std::vector<TokenVector>(temp.size());
    for (std::size_t i = 0; i < temp.size(); ++i)
    {
        TokenVector vec;
        tokenizer->numeric(temp[i], vec);
        (*clusteringContainer)[i] = vec;
    }

    
    std::vector<std::vector<int> >* similarClustering  = new std::vector<std::vector<int> >(clusteringContainer->size());
    for (std::size_t i = 0; i < clusteringContainer->size(); ++i)
    {
        pri_queue queue;
        for (std::size_t j = 0; j < clusteringContainer->size(); j++)
        {
            if (i == j)
            {
                continue;
            }
            float sim = similarity((*clusteringContainer)[i], (*clusteringContainer)[j]);
            if(queue.size() >= N_SIM)
            {
                if(sim > queue.top().second)
                {
                //std::cout<<"pop "<<queue.top().second<<"\n";
                    queue.pop();
                    queue.push(std::make_pair(j, sim));
                //std::cout<<"push "<<sim<<"\n";
                }
            }
            else
            {
                //std::cout<<"push "<<sim<<"\n";
                queue.push(std::make_pair(j, sim));
            }
        }
        
        std::vector<int> similarVector;
        while (!queue.empty())
        {
            similarVector.push_back(queue.top().first);
            queue.pop();
        }
        if (i % 11 == 0)
        {
            std::cout<<"finished "<< (float) i / clusteringContainer->size() * 100<<"\n";;
        }
        (*similarClustering)[i] = similarVector;
    }

    // flush
    std::ofstream ofs(similarFilename.c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << *similarClustering;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}
