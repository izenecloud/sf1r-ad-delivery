#ifndef ID_MAPPER_H_
#define ID_MAPPER_H_

#include <boost/unordered_map.hpp>

#include <string>
#include <utility>

namespace slim { namespace model {

class id_mapper {
public:
    std::pair<bool, int> insert(const std::string & s) {
        typedef boost::unordered_map<std::string, int> dictT;

        dictT::iterator i = _dict.find(s);
        if (i != _dict.end()) {
            return std::make_pair(false, i->second);
        } else {
            int ret = _dict.size();
            _dict.insert(std::make_pair(s, ret));
            return std::make_pair(true, ret);
        }
    }

    std::pair<bool, int> has(const std::string & s) {
        typedef boost::unordered_map<std::string, int> dictT;

        dictT::iterator i = _dict.find(s);
        if (i != _dict.end()) {
            return std::make_pair(true, i->second);
        } else {
            return std::make_pair(false, 0);
        }
    }

    int size() {
        return _dict.size();
    }

    boost::unordered_map<std::string, int> _dict;
};

}}

#endif
