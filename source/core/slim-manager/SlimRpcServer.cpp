#include "SlimRpcServer.h"

namespace sf1r { namespace slim {

SlimRpcServer::SlimRpcServer(std::vector<std::vector<int> > & similar_cluster,
                             std::vector<std::vector<int> > & similar_tareid,
                             boost::shared_mutex & rw_mutex)
    : _similar_cluster(similar_cluster),
      _similar_tareid(similar_tareid),
      _rw_mutex(rw_mutex)
{
}

SlimRpcServer::~SlimRpcServer()
{
}

void SlimRpcServer::start(const std::string & host,
                          int port,
                          int threadNum)
{
    instance.listen(host, port);
    instance.start(threadNum);
}

void SlimRpcServer::stop()
{
    instance.end();
    instance.join();
}

void SlimRpcServer::dispatch(msgpack::rpc::request req)
{
    try {
        std::string method;
        req.method().convert(&method);

        if (method == "update_similar_cluster") {
            msgpack::type::tuple<std::vector<std::vector<int> > > params;
            req.params().convert(&params);
            update_similar_cluster(req, params.get<0>());
        } else if (method == "update_similar_tareid") {
            msgpack::type::tuple<std::vector<std::vector<int> > > params;
            req.params().convert(&params);
            update_similar_tareid(req, params.get<0>());
        } else {
            req.error(msgpack::rpc::NO_METHOD_ERROR);
        }
    } catch (msgpack::type_error & e) {
        req.error(msgpack::rpc::ARGUMENT_ERROR);
        return;
    } catch (std::exception & e) {
        req.error(std::string(e.what()));
        return;
    } catch (...) {
        req.error(std::string("unknown error"));
        return;
    }
}

void SlimRpcServer::update_similar_cluster(msgpack::rpc::request req,
                                           const std::vector<std::vector<int> > & similar_cluster)
{
    boost::unique_lock<boost::shared_mutex> lock(_rw_mutex);
    _similar_cluster = similar_cluster;
    req.result(true);
}

void SlimRpcServer::update_similar_tareid(msgpack::rpc::request req,
                                          const std::vector<std::vector<int> > & similar_tareid)
{
    boost::unique_lock<boost::shared_mutex> lock(_rw_mutex);
    _similar_tareid = similar_tareid;
    req.result(true);
}

}}
