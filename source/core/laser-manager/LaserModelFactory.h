#ifndef SF1R_LASER_MODEL_FACTORY_H
#define SF1R_LASER_MODEL_FACTORY_H
#include <string>
#include <vector>

namespace sf1r {
class LaserManager;
class LaserPara;
}

namespace sf1r { namespace laser {
class AdIndexManager;
class LaserModel;

class LaserModelFactory
{
public:
    LaserModelFactory(const LaserManager& laserManager);
public:
    LaserModel* createModel(const LaserPara& para,
        const std::string& workdir) const;
 
private:
    const std::vector<std::vector<int> >& similarClustering_;
    const AdIndexManager& adIndexer_;
    const std::string sysdir_;
};

} }
#endif
