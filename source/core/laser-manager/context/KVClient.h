#ifndef SF1R_LASER_CONTEXT_KV_CLIENT_H
#define SF1R_LASER_CONTEXT_KV_CLIENT_H

namespace sf1r { namespace laser { namespace context {
class KVClient
{
public:
    KVClient(const std::string& addr, const int port)
    {
    }


public:
    bool context(const std::string& text, std::vector<std::pair<int, float> >& context) const
    {
        return false;
    }

    void shutdown()
    {
    }
};
} } }

#endif
