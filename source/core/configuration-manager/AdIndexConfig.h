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
    std::string dmp_ip;
    uint16_t dmp_port;
    std::string stream_log_ip;
    uint16_t stream_log_port;

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
        ar & dmp_ip;
        ar & dmp_port;
        ar & stream_log_ip;
        ar & stream_log_port;
    }
};

} // namespace sf1r

#endif // SF1R_AD_INDEX_CONFIG_H_
