#ifndef SF1R_LASER_INDEX_CONFIG_H_
#define SF1R_LASER_INDEX_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The configuration for <Ad Index>.
 */
class LaserIndexConfig
{
public:
    bool isEnable;

    LaserIndexConfig() : isEnable(false)
    {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
    }
};

} // namespace sf1r

#endif
