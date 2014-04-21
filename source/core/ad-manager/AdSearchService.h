#ifndef AD_SEARCH_SERVICE_H
#define AD_SEARCH_SERVICE_H


#include <util/singleton.h>
#include <vector>
#include <string>
#include <net/sf1r/distributed/Sf1Node.hpp>
#include <boost/function.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>

namespace izenelib { namespace zookeeper {
class ZooKeeper;
} }

namespace msgpack { namespace rpc {
    class session_pool;
} }

namespace sf1r
{

class KeywordSearchActionItem;
class KeywordSearchResult;
class SearchWorker;
class ServiceWatcher;

class SearchServiceNode : public izenelib::net::sf1r::Sf1Node
{
public:
    SearchServiceNode()
        : izenelib::net::sf1r::Sf1Node("", ""), master_port(0)
    {
    }
    SearchServiceNode(const std::string& path, const std::string& data)
        : izenelib::net::sf1r::Sf1Node(path, data), master_port(0)
    {
    }

    uint32_t master_port;
};

class AdSearchService
{
public:
    typedef boost::function<void()> ServerChangedCBT;
    AdSearchService(SearchWorker* local_search = NULL);
    ~AdSearchService();
    void init(const std::string& zookeeper_hosts,
        const std::string& match_master_name,
        const std::string& search_coll);

    bool search(const KeywordSearchActionItem& action, KeywordSearchResult& ret);
    bool watchServerChange(const std::string& cb_id, const ServerChangedCBT& cb);
    void unwatchServerChange(const std::string& cb_id);
    void getServiceServerList(std::map<std::string, SearchServiceNode>& server_list);

private:
    typedef std::map<std::string, ServerChangedCBT> WatcherListT;
    typedef std::map<std::string, SearchServiceNode> SearchServerListT;
    
    void reconnect();
    void rescanSearchServiceNodes();
    void scanSearchServiceNodes();
    void watchChildren(const std::string& path);
    void clearSearchServiceNodes();
    void addSearchServiceTopology(const std::string& path);
    bool addSearchServiceNode(const std::string& path, bool notify = true);
    void updateNodeData(const std::string& path, bool notify = true);
    void removeSearchServiceNode(const std::string& path, bool notify = true);
    void notifyWatcherServerChange();
    SearchWorker* ad_local_searcher_;
    boost::shared_ptr<izenelib::zookeeper::ZooKeeper> zookeeper_;
    boost::shared_ptr<ServiceWatcher> zoo_watcher_;
    std::string match_master_name_;
    std::string search_coll_;
    SearchServerListT remote_search_service_list_;
    std::auto_ptr<msgpack::rpc::session_pool> session_pool_;
    boost::shared_mutex  watcher_mutex_;
    WatcherListT watcher_list_;
    boost::shared_mutex  mutex_;
    std::size_t counter_;
    typedef boost::shared_lock<boost::shared_mutex> ReadLockT;
    typedef boost::unique_lock<boost::shared_mutex> WriteLockT;
    friend class ServiceWatcher;
};

}

#endif
