#ifndef SF1R_LASER_AD_INDEX_MANAGER_H
#define SF1R_LASER_AD_INDEX_MANAGER_H

#include <3rdparty/am/luxio/array.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <common/inttypes.h>
#include <document-manager/DocumentManager.h>

namespace sf1r { namespace laser {


class AdIndexManager
{
    typedef Lux::IO::Array ContainerType;
    typedef std::pair<docid_t, std::vector<std::pair<int, float> > > AD;
    typedef std::vector<AD> ADVector;

    typedef std::vector<ADVector> Cache;

    friend class LaserRecommend;
public:
    AdIndexManager(const std::string& workdir, 
        const std::size_t clusteringNum,
        const boost::shared_ptr<DocumentManager>& documentManager);
    ~AdIndexManager();

public:
    void index(const std::size_t& clusteringId, 
        const docid_t& docid, 
        const std::vector<std::pair<int, float> >& vec);
    bool get(const std::size_t& clusteringId, ADVector& advec) const;
    
    docid_t getLastDocId() const
    {
        return lastDocId_;
    }

    void setLastDocId(const docid_t docid)
    {
        lastDocId_ = docid;
    }
    
    void preIndex();
    void postIndex();
private:
    void open_();
    void serializeLastDocid_();
private:
    const std::string workdir_;
    const std::size_t clusteringNum_;
    ContainerType* containerPtr_;
    docid_t lastDocId_;

    const boost::shared_ptr<DocumentManager>& documentManager_;
    //Cache* cache_;
};

} }

#endif
