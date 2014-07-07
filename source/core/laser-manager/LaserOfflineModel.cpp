#include "LaserModel.h"
#include "LaserOfflineModel.h"
#include "AdIndexManager.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <glog/logging.h>

namespace sf1r { namespace laser {

LaserOfflineModel::LaserOfflineModel(const AdIndexManager& adIndexer,
    const std::string& filename,
    const std::size_t adDimension)
    : adIndexer_(adIndexer)
    , filename_(filename)
    , adDimension_(adDimension)
    , alpha_(NULL)
    , beta_(NULL)
    , betaStable_(NULL)
    , quadratic_(NULL)
    , quadraticStable_(NULL)
    , threadGroup_(NULL)
{
    alpha_ = new std::vector<float>();
    beta_ = new std::vector<float>();
    betaStable_ = new std::vector<float>(adDimension_);
    quadratic_ = new std::vector<std::vector<float> >();
    quadraticStable_ = new std::vector<std::vector<float> >(adDimension_);
    if (boost::filesystem::exists(filename_))
    {
        load();
    }
        /*std::size_t adf = 10000;
        std::size_t userf = 200;
        alpha_->resize(userf);
        beta_->resize(adf);
        quadratic_->resize(userf);
        for (std::size_t i = 0; i < adDimension_; ++i)
        {
            (*betaStable_)[i] = (rand() % 100) / 100.0;
            std::vector<float> vec;
            for (std::size_t k = 0; k < userf; ++k)
            {
                vec.push_back((rand() % 100) / 100.0);
            }
            (*quadraticStable_)[i] = vec;
        }
        for (std::size_t i = 0; i < userf; ++i)
        {
            (*alpha_)[i] = (rand() % 100) / 100.0;
        }
        for (std::size_t i = 0; i < adf; ++i)
        {
            (*beta_)[i] = (rand() % 100) / 100.0;
        }
        for (std::size_t i = 0; i < userf; ++i)
        {
            std::vector<float> vec;
            for (std::size_t k = 0; k < adf; k++)
            {
                vec.push_back((rand() % 100) / 100.0);
            }
            (*quadraticStable_)[i] = vec;
        }
        save();*/
}

LaserOfflineModel::~LaserOfflineModel()
{
    if (NULL != threadGroup_)
    {
        threadGroup_->join_all();
        delete threadGroup_;
    }
    if (NULL != alpha_)
    {
        delete alpha_;
        alpha_ = NULL;
    }
    if (NULL != beta_)
    {
        delete beta_;
        beta_ = NULL;
    }
    if (NULL != betaStable_)
    {
        delete betaStable_;
        betaStable_ = NULL;
    }
    if (NULL != quadratic_)
    {
        delete quadratic_;
        quadratic_ = NULL;
    }
    if (NULL != quadraticStable_)
    {
        delete quadraticStable_;
        quadraticStable_ = NULL;
    }
}

float LaserOfflineModel::score( 
    const std::string& text,
    const std::vector<std::pair<int, float> >& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score; 
    // x * alpha
    ret += dot(*alpha_, user);
    
    // c * beta
    if (ad.first < betaStable_->size())
    {
        ret += (*betaStable_)[ad.first];
    }
    else
    {
        return ret;
    }
    
    // x * A * c
    if (ad.first < quadraticStable_->size())
    {
        ret += dot((*quadraticStable_)[ad.first], user);
    }
    else
    {
        return ret;
    }
    return ret;
}

float LaserOfflineModel::score( 
    const std::string& text,
    const std::vector<float>& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score; 
    // x * alpha
    ret += dot(*alpha_, user);
    
    // c * beta
    if (ad.first < betaStable_->size())
    {
        ret += (*betaStable_)[ad.first];
    }
    else
    {
        return ret;
    }
    
    // x * A * c
    if (ad.first < quadraticStable_->size())
    {
        ret += dot((*quadraticStable_)[ad.first], user);
    }
    else
    {
        return ret;
    }
    return ret;
}

void LaserOfflineModel::save()
{
    std::ofstream ofs(filename_.c_str(), std::ofstream::binary | std::ofstream::trunc);
    boost::archive::text_oarchive oa(ofs);
    try
    {
        oa << alpha_;
        oa << beta_;
        oa << beta_;
        oa << quadratic_;
        oa << quadraticStable_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ofs.close();
}

void LaserOfflineModel::load()
{
    std::ifstream ifs(filename_.c_str(), std::ios::binary);
    boost::archive::text_iarchive ia(ifs);
    try
    {
        ia >> alpha_;
        ia >> beta_;
        ia >> betaStable_;
        ia >> quadratic_;
        ia >> quadraticStable_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ifs.close();
}
    
void LaserOfflineModel::updateAdDimension(const std::size_t adDimension)
{
    betaStable_->resize(adDimension);
    quadraticStable_->resize(adDimension);
    if (NULL != threadGroup_)
    {
        LOG(INFO)<<"wait last time's pre-compute threads finish";
        threadGroup_->join_all();
        delete threadGroup_;
    }
    threadGroup_ = new boost::thread_group();
    for (int i = 0; i < THREAD_NUM; ++i)
    {
        threadGroup_->create_thread(
            boost::bind(&LaserOfflineModel::precompute, this,
                        adDimension_, adDimension, i));
    }
    adDimension_ = adDimension;
}
    
void LaserOfflineModel::precompute(std::size_t startId, std::size_t endId, int threadId)
{
    for (std::size_t adId = startId; adId < endId; ++adId)
    {
        if (0 != adId % THREAD_NUM)
            continue;

        std::vector<std::pair<int, float> > vec;
        adIndexer_.get(adId, vec);
        (*betaStable_)[adId] = dot(*beta_, vec);
        
        std::vector<float>& row = (*quadraticStable_)[adId];
        std::vector<std::vector<float> >::const_iterator it = quadratic_->begin();
        for (; it != quadratic_->end(); ++it)
        {
            row.push_back(dot(*it, vec));
        }
    }
}
    
} }
