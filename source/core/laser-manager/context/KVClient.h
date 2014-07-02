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
        for (std::size_t i = 0; i < 200; ++i)
        {
            context.push_back(std::make_pair(i, (rand() % 100) / 100.0));
        }
        return true;
    }

    void shutdown()
    {
    }
};
} } }

#endif
