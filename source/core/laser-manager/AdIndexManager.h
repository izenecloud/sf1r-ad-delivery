#ifndef SF1R_LASER_AD_INDEX_MANAGER_H
#define SF1R_LASER_AD_INDEX_MANAGER_H

#include <3rdparty/am/luxio/array.h>
#include <vector>
namespace sf1r { namespace laser {

class AdIndexManager
{
    typedef Lux::IO::Array ContainerType;
    typedef std::pair<std::string, std::vector<std::pair<int, float> > > AD;
    typedef std::vector<AD> ADVector;
public:
    AdIndexManager(const std::string& workdir);
    ~AdIndexManager();

public:
    void index(const std::size_t& clusteringId, 
        const std::string& docid, 
        const std::vector<std::pair<int, float> >& vec);
private:
    void open_();
private:
    const std::string filename_;
    ContainerType* containerPtr_;
};

} }

#endif
