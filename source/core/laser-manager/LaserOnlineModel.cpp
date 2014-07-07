#include "LaserModel.h"
#include "LaserOnlineModel.h"
namespace sf1r { namespace laser {
    
float LaserOnlineModel::score( 
    const std::string& text,
    const std::vector<std::pair<int, float> >& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    if (!user.empty())
    {
        ret += dot(eta_, user);
    }
    else
    {
        ret += dot(eta_, ad.second);
    }
    ret += delta_;
    return ret;
}

float LaserOnlineModel::score( 
    const std::string& text,
    const std::vector<float>& user, 
    const std::pair<docid_t, std::vector<std::pair<int, float> > >& ad,
    const float score) const
{
    float ret = score;
    if (!user.empty())
    {
        ret += dot(eta_, user);
    }
    else
    {
        ret += dot(eta_, ad.second);
    }
    ret += delta_;
    return ret;
}

} }
