#ifndef SF1R_AD_INDEX_CONFIG_H_
#define SF1R_AD_INDEX_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The configuration for <Ad Index>.
 */
class AdIndexConfig
{
public:
    bool isEnable;

    std::string indexFilePath;

    std::string clickPredictorWorkingPath;

    bool enable_selector;
    bool enable_rec;
    bool enable_sponsored_search;
    AdIndexConfig() : isEnable(false)
                      , enable_selector(false)
                      , enable_rec(false)
                      , enable_sponsored_search(false)
    {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & indexFilePath;
        ar & clickPredictorWorkingPath;
        ar & enable_selector;
        ar & enable_rec;
        ar & enable_sponsored_search;
    }
};

} // namespace sf1r

#endif // SF1R_AD_INDEX_CONFIG_H_
