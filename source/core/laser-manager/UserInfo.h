#ifndef SF1R_LASER_USER_INFO_H
#define SF1R_LASER_USER_INFO_H
#include "SparseVector.h"
namespace sf1r { namespace laser {
class UserInfo
{
public:
    std::vector<int>& roughIndex()
    {
        return rough_.index();
    }

    std::vector<float>& roughValue()
    {
        return rough_.value();
    }

    std::vector<int>& subtleIndex()
    {
        return subtle_.index();
    }

    std::vector<float>& subtleValue()
    {
        return subtle_.value();
    }

    std::vector<int>& urlIndex()
    {
        return url_.index();
    }

    std::vector<float>& urlValue()
    {
        return url_.value();
    }
private:
    // url
    SparseVector url_;
    // page content information
    SparseVector rough_;
    SparseVector subtle_;
};

} }
#endif
