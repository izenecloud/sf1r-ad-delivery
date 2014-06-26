#ifndef SF1R_SLIM_RPC_SERVER_H
#define SF1R_SLIM_RPC_SERVER_H

#include <3rdparty/msgpack/rpc/server.h>
#include <boost/shared_ptr.hpp>

namespace sf1r { namespace slim {

class SlimRpcServer: public msgpack::rpc::server::base {
public:
    SlimRpcServer(std::vector<std::vector<int> > & similar_cluster,
                  std::vector<std::vector<int> > & similar_tareid,
                  boost::shared_mutex & rw_mutex);
    ~SlimRpcServer();

public:
    void start(const std::string & host,
               int port,
               int threadNum);

    void stop();

    void dispatch(msgpack::rpc::request req);

    void update_similar_cluster(msgpack::rpc::request req,
                                const std::vector<std::vector<int> > & similar_cluster);

    void update_similar_tareid(msgpack::rpc::request req,
                                const std::vector<std::vector<int> > & similar_tareid);

private:
    std::vector<std::vector<int> > & _similar_cluster;
    std::vector<std::vector<int> > & _similar_tareid;
    boost::shared_mutex & _rw_mutex;
};

}}

#endif
