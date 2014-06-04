#ifndef SF1R_LASER_MODEL_FACTORY_H
#define SF1R_LASER_MODEL_FACTORY_H
#include <string>
#include <vector>

namespace sf1r {
class LaserManager;
class LaserIndexConfig;
}

namespace sf1r { namespace laser {
class AdIndexManager;
class LaserModel;

class LaserModelFactory
{
public:
    LaserModelFactory(const LaserManager& laserManager);
public:
    LaserModel* createModel(const LaserIndexConfig& config,
        const std::string& workdir,
        const std::size_t ncandidate) const;
 
private:
    const std::vector<std::vector<int> >& similarClustering_;
    const AdIndexManager& adIndexer_;
};

} }
#endif
