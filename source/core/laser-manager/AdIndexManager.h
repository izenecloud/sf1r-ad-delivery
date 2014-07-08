#ifndef SF1R_LASER_AD_INDEX_MANAGER_H
#define SF1R_LASER_AD_INDEX_MANAGER_H

#include <3rdparty/am/luxio/array.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <common/inttypes.h>
#include <document-manager/DocumentManager.h>

namespace sf1r {
class LaserManager;
}
namespace sf1r { namespace laser {

class AdIndexManager
{
    typedef std::pair<docid_t, std::vector<std::pair<int, float> > > AD;
    typedef std::vector<AD> ADVector;

    friend class LaserRecommend;
public:
    AdIndexManager(const std::string& workdir,
        const bool isEnableClustering, 
        LaserManager* laserManager);
    ~AdIndexManager();

public:
    void index(const docid_t& docid, const std::vector<std::pair<int, float> >& vec);
    bool get(const docid_t& docid, std::vector<std::pair<int, float> >& vec) const;
    
    // for clustering system
    void index(const std::size_t& clusteringId, 
        const docid_t& docid,
        const std::vector<std::pair<int, float> >& vec);
    
    bool get(const std::size_t& clusteringId, std::vector<docid_t>& docids) const;
    bool get(const docid_t& docid, std::size_t& clustering) const;
    
    bool get(const std::size_t& clusteringId, ADVector& adList) const;
    
    docid_t getLastDocId() const
    {
        return lastDocId_;
    }

    void setLastDocId(const docid_t docid)
    {
        lastDocId_ = docid;
    }
    
    bool convertDocId(const std::string& docStr, docid_t& docId) const;
    
    void preIndex();
    void postIndex();
private:
    void loadClusteringIndex();
    void saveClusteringIndex();
    void loadAdIndex();
    void saveAdIndex();
private:
    const std::string workdir_;
    const bool isEnableClustering_;
    std::vector<std::vector<std::pair<int, float> > >* adPtr_;
    std::vector<std::size_t>* adClusteringPtr_;
    std::vector<std::vector<docid_t> >* clusteringPtr_;
    docid_t lastDocId_;
    mutable boost::shared_mutex mtx_;

    LaserManager* laserManager_;
    const boost::shared_ptr<DocumentManager>& documentManager_;
};

} }

#endif
