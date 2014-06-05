#include "LaserRecommend.h"
#include "LaserModel.h"
#include "LaserModelFactory.h"
#include "LaserManager.h"
#include <glog/logging.h>


namespace sf1r { namespace laser {

LaserRecommend::LaserRecommend(const LaserManager* laserManager)
    : laserManager_(laserManager)
    , factory_(NULL)
    , model_(NULL)
{
    factory_ = new LaserModelFactory(*laserManager_);
    model_ = factory_->createModel(laserManager_->config_, 
        laserManager_->workdir_ + "/model/", 1000);
}

LaserRecommend::~LaserRecommend()
{
    if (NULL != factory_)
    {
        delete factory_;
        factory_ = NULL;
    }
    if (NULL != model_)
    {
        delete model_;
        model_ = NULL;
    }
}

bool LaserRecommend::recommend(const std::string& text, 
    std::vector<docid_t>& itemList, 
    std::vector<float>& itemScoreList, 
    const std::size_t num) const
{
    std::vector<std::pair<int, float> > context;
    extractContext(text, context);

    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > > ad;
    std::vector<float> score;
    if (!model_->candidate(text, context, ad, score))
    {
        return false;
    }

    priority_queue queue;
    for (std::size_t i = 0; i < ad.size(); ++i)
    {
        topn(ad[i].first, model_->score(text, context,  ad[i], score[i]), num, queue);   
    }
    {
        while (!queue.empty())
        {
           itemList.push_back(queue.top().first);
           itemScoreList.push_back(queue.top().second);
           queue.pop();
        }
    }
    return true;
}

void LaserRecommend::extractContext(const std::string& text, std::vector<std::pair<int, float> >& context) const
{
}

void LaserRecommend::topn(const docid_t& docid, const float score, const std::size_t n, priority_queue& queue) const
{
    if(queue.size() >= n)
    {
        if(score > queue.top().second)
        {
            queue.pop();
            queue.push(std::make_pair(docid, score));
        }
    }
    else
    {
        queue.push(std::make_pair(docid, score));
    }
}

void LaserRecommend::dispatch(const std::string& method, msgpack::rpc::request& req)
{
    model_->dispatch(method, req);
}
    
} }
