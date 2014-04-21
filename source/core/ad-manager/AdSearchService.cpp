#include "AdSearchService.h"
#include <aggregator-manager/MasterServerConnector.h>
#include <aggregator-manager/SearchWorker.h>
#include <query-manager/ActionItem.h>
#include <query-manager/SearchKeywordOperation.h>
#include <common/ResultType.h>
#include <boost/regex.hpp>
#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>
#include <3rdparty/zookeeper/ZooKeeperWatcher.hpp>
#include <util/kv2string.h>
#include <3rdparty/msgpack/rpc/session.h>
#include <3rdparty/msgpack/rpc/session_pool.h>

using namespace izenelib::zookeeper;
using namespace izenelib::util;
using namespace izenelib::net::sf1r;

namespace sf1r
{

static const std::string ROOT_NODE = "/";
static const std::string  TOPOLOGY = "/Servers";
static const boost::regex SF1R_ROOT_REGEX("\\/SF1R-[^\\/]+");
static const boost::regex TOPOLOGY_REGEX("\\/SF1R-[^\\/]+\\/Servers");
static const boost::regex SF1R_NODE_REGEX("\\/SF1R-[^\\/]+\\/Servers\\/Server\\d+");
static const std::string SearchService = "search";
static const std::string       HOST_KEY = "host";

static const std::string     BAPORT_KEY = "baport";
static const std::string MASTERPORT_KEY = "masterport";
static const std::string MASTER_NAME_KEY = "mastername";
static const std::string COLLECTION_KEY = "collection";
static const std::string SERVICE_STATE_KEY = "service_state";
static const std::string SERVICE_NAMES_KEY = "service_names";

static const char  DELIMITER_CHAR = ',';

class ServiceWatcher : public ZooKeeperEventHandler, private boost::noncopyable
{
public:
    ServiceWatcher(AdSearchService& service)
        :service_(service)
    {
    }
    ~ServiceWatcher()
    {
    }
    void process(ZooKeeperEvent& zkEvent)
    {
        LOG(INFO) << zkEvent.toString();
        if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_EXPIRED_SESSION_STATE)
        {
            while(true)
            {
                try
                {
                    LOG(INFO) << "session expired, reconnect.";
                    service_.reconnect();
                    break;
                }
                catch(...)
                {
                    sleep(10);
                }
            }
        }
        else if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
        {
            LOG(INFO) << "auto connected";
            service_.rescanSearchServiceNodes();
        }
    }

    void onNodeCreated(const std::string& path)
    {
        LOG(INFO) << "created: " << path;
        if (boost::regex_match(path, TOPOLOGY_REGEX)) {
            LOG(INFO) << "adding " << path << " ...";
            service_.addSearchServiceTopology(path);
        }
    }

    void onNodeDeleted(const std::string& path)
    {
        LOG(INFO) << "deleted: " << path;
        if (boost::regex_match(path, SF1R_NODE_REGEX)) {
            LOG(INFO) << "removing " << path << " ...";
            service_.removeSearchServiceNode(path);
        } else if (boost::regex_match(path, TOPOLOGY_REGEX)) {
            LOG(INFO) << "watching " << path << " ...";
            service_.watchChildren(path);
        }
    }

    void onDataChanged(const std::string& path)
    {
        LOG(INFO) << "changed: " << path;
        if (boost::regex_match(path, SF1R_NODE_REGEX)) {
            LOG(INFO) << "reloading " << path << " ...";
            service_.updateNodeData(path);
        }
    }
    void onChildrenChanged(const std::string& path)
    {
        LOG(INFO) << "children changed: " << path;
        if (boost::regex_search(path, SF1R_ROOT_REGEX) || path == ROOT_NODE) {
            service_.watchChildren(path);
        }
    }

    void onSessionClosed() {};
    void onSessionConnected() {};
    void onSessionExpired() {};
    void onAuthFailed() {};
private:
    AdSearchService& service_;
};

AdSearchService::AdSearchService(SearchWorker* local_search)
    :ad_local_searcher_(local_search)
{
}

AdSearchService::~AdSearchService()
{
    clearSearchServiceNodes();
    LOG(INFO) << "AdSearchService closed";
}

void AdSearchService::init(const std::string& zookeeper_hosts,
    const std::string& match_master_name,
    const std::string& search_coll)
{
    search_coll_ = search_coll;
    match_master_name_ = match_master_name;
    LOG(INFO) << "connecting to zookeeper servers: " << zookeeper_hosts << ", match master: " << match_master_name;
    if (zookeeper_hosts.empty())
    {
        LOG(INFO) << "remote search service disabled.";
        return;
    }
    int timeout =  10*1000;
    zookeeper_.reset(new ZooKeeper(zookeeper_hosts, timeout, true));

    while (!zookeeper_->isConnected()) {
        timeout -= 100;
        if (timeout < 0)
        {
            throw ZooKeeperException("Not connected to (servers): " + zookeeper_hosts);
        }
        usleep(100*1000);
    }
    
    LOG(INFO) << "Registering watcher ...";
    zoo_watcher_.reset(new ServiceWatcher(*this));
    zookeeper_->registerEventHandler(zoo_watcher_.get());
    
    scanSearchServiceNodes();

    session_pool_.reset(new msgpack::rpc::session_pool());
    session_pool_->start(10);
}

void AdSearchService::reconnect()
{
    LOG(INFO) << "reconnecting...";
    if (!zookeeper_)
        return;
    zookeeper_->disconnect();
    zookeeper_->connect();
    int timeout = 10*1000;
    while (!zookeeper_->isConnected())
    {
        timeout -= 100;
        if (timeout < 0)
        {
            throw ZooKeeperException("reconnect failed.");
        }
        usleep(100*1000);
    }
    clearSearchServiceNodes();
    scanSearchServiceNodes();
}

void AdSearchService::getServiceServerList(std::map<std::string, SearchServiceNode>& server_list)
{
    ReadLockT lock(mutex_);
    server_list = remote_search_service_list_;
}

void AdSearchService::rescanSearchServiceNodes()
{
    if (zookeeper_ && zookeeper_->isConnected())
    {
        clearSearchServiceNodes();
        scanSearchServiceNodes();
    }
}

void AdSearchService::scanSearchServiceNodes()
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return;
    std::vector<std::string> clusters;
    zookeeper_->getZNodeChildren(ROOT_NODE, clusters, ZooKeeper::WATCH);
    BOOST_FOREACH(const std::string& s, clusters)
    {
        if (boost::regex_match(s, SF1R_ROOT_REGEX))
        {
            LOG(INFO) << "node: " << s;
            addSearchServiceTopology(s + TOPOLOGY);
        }
    }
    LOG(INFO) << "found total search service nodes: " << remote_search_service_list_.size();
}

void AdSearchService::watchChildren(const std::string& path)
{
    if (!zookeeper_->isZNodeExists(path, ZooKeeper::WATCH))
    {
        return;
    }
    if (!boost::regex_search(path, TOPOLOGY_REGEX) &&
        !boost::regex_match(path, SF1R_ROOT_REGEX) &&
        path != ROOT_NODE)
    {
        zookeeper_->isZNodeExists(path, ZooKeeper::NOT_WATCH);
        LOG(INFO) << "Not watching children of : " << path;
        return;
    }
    std::vector<std::string> children;
    zookeeper_->getZNodeChildren(path, children, ZooKeeper::WATCH);
    BOOST_FOREACH(const string& s, children) {
        LOG(INFO) << "child: " << s;
        if (boost::regex_match(s, SF1R_NODE_REGEX)) {
            LOG(INFO) << "Adding node: " << s;
            addSearchServiceNode(s);
        } else if (boost::regex_search(s, SF1R_ROOT_REGEX)) {
            watchChildren(s);
        }
    }
}

void AdSearchService::clearSearchServiceNodes()
{
    {
        WriteLockT lock(mutex_);
        LOG(INFO) << "clearing the search nodes.";
        remote_search_service_list_.clear();
    }
    notifyWatcherServerChange();
}

void AdSearchService::addSearchServiceTopology(const std::string& path)
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return;
    if (!zookeeper_->isZNodeExists(path, ZooKeeper::WATCH))
    {
        return;
    }
    std::vector<std::string> replicas;
    zookeeper_->getZNodeChildren(path, replicas, ZooKeeper::WATCH);
    if (replicas.empty())
        return;
    BOOST_FOREACH(const std::string& replica, replicas)
    {
        addSearchServiceNode(replica, false);
        std::vector<std::string> nodes;
        zookeeper_->getZNodeChildren(replica, nodes, ZooKeeper::WATCH);
        
        if (nodes.empty())
        {
            continue;
        }
        BOOST_FOREACH(const std::string& path, nodes)
        {
            addSearchServiceNode(path, false);
        }
    }
    notifyWatcherServerChange();
}

bool AdSearchService::addSearchServiceNode(const std::string& path, bool notify)
{
    {
        ReadLockT lock(mutex_);
        if (remote_search_service_list_.find(path) != remote_search_service_list_.end())
        {
            LOG(INFO) << "node already exists. " << path;
            return true;
        }
    }
    std::string nodedata;
    zookeeper_->getZNodeData(path, nodedata, ZooKeeper::WATCH);

    kv2string parser;
    parser.loadKvString(nodedata);

    if (parser.hasKey(MASTERPORT_KEY))
    {
        LOG(INFO) << "no master port key.";
        return false;
    }
    if (!match_master_name_.empty())
    {
        std::string mastername = parser.getStrValue(MASTER_NAME_KEY);
        if(mastername != match_master_name_) { //match special master SF1 node
            LOG(INFO) << "ignore not matched master : " << mastername;
            return false;
        }
    }
    // check for search service
    std::string service_names;
    service_names = parser.getStrValue(SERVICE_NAMES_KEY);
    if (service_names.find(SearchService) == std::string::npos)
    {
        LOG(INFO) << "current node has no search service : " << path;
        return false;
    }

    std::string coll_str = parser.getStrValue(SearchService + COLLECTION_KEY);
    if (coll_str.find(search_coll_) != std::string::npos)
    {
        WriteLockT lock(mutex_);
        SearchServiceNode node(path, nodedata);
        node.master_port = parser.getUInt32Value(MASTERPORT_KEY);
        remote_search_service_list_.insert(SearchServerListT::value_type(path, node));
        LOG(INFO) << "server added: " << path;
    }
    if (notify)
        notifyWatcherServerChange();
    return true;
}

void AdSearchService::updateNodeData(const std::string& path, bool notify)
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return;
    std::string data;
    WriteLockT lock(mutex_);
    SearchServerListT::iterator it = remote_search_service_list_.find(path);
    if (it == remote_search_service_list_.end())
        return;
    zookeeper_->getZNodeData(path, data, ZooKeeper::WATCH);
    kv2string parser;
    parser.loadKvString(data);
    std::string  new_coll = parser.getStrValue(SearchService + COLLECTION_KEY);
    if (new_coll.find(search_coll_) == std::string::npos)
    {
        LOG(INFO) << "removing the sf1r node since no longer collection: " << new_coll;
        remote_search_service_list_.erase(it);
    }
    else
    {
        LOG(INFO) << "server updated: " << path;
        SearchServiceNode node(path, data);
        node.master_port = parser.getUInt32Value(MASTERPORT_KEY);
        it->second = node;
    }
    if (notify)
        notifyWatcherServerChange();
    
}

void AdSearchService::removeSearchServiceNode(const std::string& path, bool notify)
{
    {
        WriteLockT lock(mutex_);
        if (remote_search_service_list_.find(path) == remote_search_service_list_.end())
            return;
        remote_search_service_list_.erase(path);
        LOG(INFO) << "server removed: " << path;
    }
    notifyWatcherServerChange();
}

bool AdSearchService::watchServerChange(const std::string& cb_id, const ServerChangedCBT& cb)
{
    WriteLockT lock(watcher_mutex_);
    if (watcher_list_.find(cb_id) != watcher_list_.end())
    {
        LOG(INFO) << "watcher already exist: " << cb_id;
        return false;
    }
    watcher_list_[cb_id] = cb;
    return true;
}

void AdSearchService::unwatchServerChange(const std::string& cb_id)
{
    WriteLockT lock(watcher_mutex_);
    watcher_list_.erase(cb_id);
}

void AdSearchService::notifyWatcherServerChange()
{
    LOG(INFO) << "begin notify server change.";
    ReadLockT lock(watcher_mutex_);

    WatcherListT::const_iterator it = watcher_list_.begin();
    for(; it != watcher_list_.end(); ++it)
    {
        it->second();
    }
}

bool AdSearchService::search(const KeywordSearchActionItem& action, KeywordSearchResult& ret)
{
    if (remote_search_service_list_.empty())
    {
        LOG(INFO) << "no remote search server.";
        if (ad_local_searcher_)
        {
            return ad_local_searcher_->doLocalSearch(action, ret);
        }
        return false;
    }
    SearchServiceNode node;
    {
        ReadLockT lock(mutex_);
        ++counter_;
        SearchServerListT::const_iterator it = remote_search_service_list_.begin();
        std::size_t index = counter_ % remote_search_service_list_.size();
        for(; it != remote_search_service_list_.end(); ++it)
        {
            if (index == 0)
                break;
            --index;
        }
        node = it->second;
    }

    try
    {
        msgpack::rpc::session s = session_pool_->get_session(node.getHost(), node.master_port);
        msgpack::rpc::future f = s.call(MasterServerConnector::Methods_[MasterServerConnector::Method_documentSearch_], action);
        ret = f.get<KeywordSearchResult>();
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << "search rpc call error :" << e.what();
        return false;
    }

    return true;
}


}
