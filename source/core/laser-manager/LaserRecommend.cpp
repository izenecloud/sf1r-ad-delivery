#include "LaserRecommend.h"
#include "LaserModel.h"
#include "LaserModelFactory.h"
#include "LaserManager.h"
#include <glog/logging.h>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace sf1r { namespace laser {

LaserRecommend::LaserRecommend(const LaserManager* laserManager)
    : laserManager_(laserManager)
    , factory_(NULL)
    , model_(NULL)
{
    factory_ = new LaserModelFactory(*laserManager_);
    model_ = factory_->createModel(laserManager_->para_, 
        laserManager_->workdir_ + "/model/");
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
    if (!model_->context(text, context))
    {
        return false;
    }
    boost::posix_time::ptime stime = boost::posix_time::microsec_clock::local_time();
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > > ad;
    std::vector<float> score;
    if (!model_->candidate(text, 1000, context, ad, score))
    {
        return false;
    }
    boost::posix_time::ptime etime = boost::posix_time::microsec_clock::local_time();
    LOG(INFO)<<"candidate time = "<<(etime-stime).total_milliseconds()<<"\t ad size = "<<ad.size();
   
    priority_queue queue;
    stime = boost::posix_time::microsec_clock::local_time();
    for (std::size_t i = 0; i < ad.size(); ++i)
    {
        //topn(ad[i].first, model_->score(text, context,  ad[i], score[i]), num, queue);   
        model_->score(text, context,  ad[i], score[i]);   
    }
    etime = boost::posix_time::microsec_clock::local_time();
    LOG(INFO)<<"score time = "<<(etime-stime).total_milliseconds();
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
