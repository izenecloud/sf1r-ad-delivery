#include "LaserModel.h"
#include "LaserOnlineModel.h"
namespace sf1r { namespace laser {
    
bool LaserOnlineModel::candidate(
    const std::string& text,
    const std::size_t ncandidate,
    const std::vector<std::pair<int, float> >& context, 
    std::vector<std::pair<docid_t, std::vector<std::pair<int, float> > > >& ad,
    std::vector<float>& score) const
{
    return true;
}

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

} }
