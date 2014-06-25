#include "server/rpc_server.h"

int main()
{
    slim::server::rpc_server server("l2");
    server.instance.listen("127.0.0.1", 19850);
    server.instance.run(30);

    return 0;
}
