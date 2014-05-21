/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef sf1r_B5MEvent_hh__
#define sf1r_B5MEvent_hh__

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <avro/Boost.hh>
#include <avro/Exception.hh>
#include <avro/AvroSerialize.hh>
#include <avro/AvroParse.hh>
#include <avro/Layout.hh>

namespace sf1r {

/*----------------------------------------------------------------------------------*/

struct Map_of_string {
    typedef std::string ValueType;
    typedef std::map<std::string, ValueType> MapType;
    typedef ValueType* (*GenericSetter)(Map_of_string *, const std::string &);
    
    Map_of_string() :
        value(),
        genericSetter(&Map_of_string::genericSet)
    { }

    void addValue(const std::string &key, const ValueType &val) {
        value.insert(MapType::value_type(key, val));
    }

    static ValueType *genericSet(Map_of_string *map, const std::string &key) { 
        map->value[key] = ValueType();
        return &(map->value[key]);
    }

    MapType value;
    GenericSetter genericSetter;

};

template <typename Serializer>
inline void serialize(Serializer &s, const Map_of_string &val, const boost::true_type &) {
    if(val.value.size()) {
        s.writeMapBlock(val.value.size());
        Map_of_string::MapType::const_iterator iter = val.value.begin();
        Map_of_string::MapType::const_iterator end  = val.value.end();
        while(iter!=end) {
            serialize(s, iter->first);
            serialize(s, iter->second);
            ++iter;
        }
    }
    s.writeMapEnd();
}

template <typename Parser>
inline void parse(Parser &p, Map_of_string &val, const boost::true_type &) {
    val.value.clear();
    while(1) {
        int size = p.readMapBlockSize();
        if(size > 0) {
            while (size-- > 0) { 
                std::string key;
                parse(p, key);
                Map_of_string::ValueType m;
                parse(p, m);
                val.value.insert(Map_of_string::MapType::value_type(key, m));
            }
        }
        else {
            break;
        }
    } 
}

class Map_of_string_Layout : public avro::CompoundLayout {
  public:
    Map_of_string_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(Map_of_string, genericSetter)));
        add(new avro::PrimitiveLayout);
    }
}; 


/*----------------------------------------------------------------------------------*/

struct B5MEvent {

    B5MEvent () :
        timestamp(),
        remoteAddress(),
        forwardAddress(),
        userAgent(),
        args(),
        internalArgs(),
        body(),
        match()
    { }

    int64_t timestamp;
    std::string remoteAddress;
    std::string forwardAddress;
    std::string userAgent;
    Map_of_string args;
    Map_of_string internalArgs;
    std::string body;
    bool match;
};

template <typename Serializer>
inline void serialize(Serializer &s, const B5MEvent &val, const boost::true_type &) {
    s.writeRecord();
    serialize(s, val.timestamp);
    serialize(s, val.remoteAddress);
    serialize(s, val.forwardAddress);
    serialize(s, val.userAgent);
    serialize(s, val.args);
    serialize(s, val.internalArgs);
    serialize(s, val.body);
    serialize(s, val.match);
    s.writeRecordEnd();
}

template <typename Parser>
inline void parse(Parser &p, B5MEvent &val, const boost::true_type &) {
    p.readRecord();
    parse(p, val.timestamp);
    parse(p, val.remoteAddress);
    parse(p, val.forwardAddress);
    parse(p, val.userAgent);
    parse(p, val.args);
    parse(p, val.internalArgs);
    parse(p, val.body);
    parse(p, val.match);
    p.readRecordEnd();
}

class B5MEvent_Layout : public avro::CompoundLayout {
  public:
    B5MEvent_Layout(size_t offset = 0) :
        CompoundLayout(offset)
    {
        add(new avro::PrimitiveLayout(offset + offsetof(B5MEvent, timestamp)));
        add(new avro::PrimitiveLayout(offset + offsetof(B5MEvent, remoteAddress)));
        add(new avro::PrimitiveLayout(offset + offsetof(B5MEvent, forwardAddress)));
        add(new avro::PrimitiveLayout(offset + offsetof(B5MEvent, userAgent)));
        add(new Map_of_string_Layout(offset + offsetof(B5MEvent, args)));
        add(new Map_of_string_Layout(offset + offsetof(B5MEvent, internalArgs)));
        add(new avro::PrimitiveLayout(offset + offsetof(B5MEvent, body)));
        add(new avro::PrimitiveLayout(offset + offsetof(B5MEvent, match)));
    }
}; 



} // namespace sf1r

namespace avro {

template <> struct is_serializable<sf1r::B5MEvent> : public boost::true_type{};
template <> struct is_serializable<sf1r::Map_of_string> : public boost::true_type{};

} // namespace avro

#endif // sf1r_B5MEvent_hh__
