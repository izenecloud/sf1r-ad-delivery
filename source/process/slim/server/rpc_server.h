#ifndef RPC_SERVER_H_
#define RPC_SERVER_H_

#include "../model/learner.h"

#include <3rdparty/msgpack/rpc/server.h>
#include <string>

namespace slim { namespace server {

class rpc_server: public msgpack::rpc::server::base {
public:
    rpc_server(const std::string & penalty_type)
        : _learner(penalty_type)
    {
    }

    void dispatch(msgpack::rpc::request req) {
        try {
            std::string method;
            req.method().convert(&method);

            if (method == "add_feedback") {
                msgpack::type::tuple<std::string, std::string> params;
                req.params().convert(&params);
                add_feedback(req, params.get<0>(), params.get<1>());
            } else if (method == "train") {
                msgpack::type::tuple<int, int> params;
                req.params().convert(&params);
                train(req, params.get<0>(), params.get<1>());
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

    void add_feedback(msgpack::rpc::request req, const std::string & user, const std::string & item) {
        req.result(true);
        _learner.add_feedback(user, item);
    }

    void train(msgpack::rpc::request req, int thread_num, int top_n) {
        req.result(true);
        _learner.coo_to_csr();
        _learner.train(thread_num, top_n);
    }

private:
    slim::model::learner _learner;
};

}}

#endif
