#include "LaserModel.h"
#include "LaserOfflineModel.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <glog/logging.h>

namespace sf1r { namespace laser {

LaserOfflineModel::LaserOfflineModel(const std::string& filename,
    const std::size_t adDimension)
    : filename_(filename)
    , adDimension_(adDimension)
    , alpha_(NULL)
    , beta_(NULL)
    , betaStable_(NULL)
    , quadratic_(NULL)
    , quadraticStable_(NULL)
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

bool LaserOfflineModel::candidate(
    const std::string& text,
        const std::size_t ncandidate,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    return false;
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
        //ret += dot(*beta_, ad.second);
    }
    
    // x * A * c
    if (ad.first < quadraticStable_->size())
    {
        ret += dot((*quadraticStable_)[ad.first], user);
    }
    else
    {
        return ret;
        //for (std::size_t i = 0; i < user.size(); ++i)
        //{
        //    std::size_t row = user[i].first;
        //    ret += user[i].second + dot((*quadratic_)[row], ad.second);
        //}
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
    
    
} }
