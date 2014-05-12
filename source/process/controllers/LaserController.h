#ifndef SF1R_LASER_CONTROLLER_H
#define SF1R_LASER_CONTROLLER_H

#include "Sf1Controller.h"
namespace sf1r
{
class LaserController : public Sf1Controller
{
public:
    void recommend();
};
}
#endif
