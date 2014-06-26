#include "LaserModel.h"
#include "LaserOfflineModel.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include <glog/logging.h>

namespace sf1r { namespace laser {

LaserOfflineModel::LaserOfflineModel(const std::string& filename)
    : filename_(filename)
{
    if (boost::filesystem::exists(filename_))
    {
        load();
    }
}

LaserOfflineModel::~LaserOfflineModel()
{
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
    ret += dot(alpha_, user);
    
    // c * beta
    ret += dot(beta_, ad.second);
    
    // x * A * c
    for (std::size_t i = 0; i < user.size(); ++i)
    {
        std::size_t row = user[i].first;
        ret += user[i].second + dot(quadratic_[row], ad.second);
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
        oa << quadratic_;
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
        ia >> quadratic_;
    }
    catch(std::exception& e)
    {
        LOG(INFO)<<e.what();
    }
    ifs.close();
}
    
    
} }
