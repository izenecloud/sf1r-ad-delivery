/*
 * test_message_pack.cpp
 *
 *  Created on: Apr 4, 2014
 *      Author: alex
 */

#include <3rdparty/am/leveldb/db.h>
#include <am/leveldb/Table.h>

#include <util/Int2String.h>

#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#include <cstdlib>   // for rand()
#include <cctype>    // for isalnum()
#include <algorithm> // for back_inserter

#include <boost/timer.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "common/utils.h"
#include <boost/test/unit_test.hpp>
#include "type/ClusteringDataStorage.h"
#include "service/DataType.h"
using namespace std;
using namespace leveldb;
using namespace clustering::type;
using namespace clustering;

#include <3rdparty/msgpack/msgpack/type/tuple.hpp>

namespace bfs = boost::filesystem;
int main()
{
    ClusteringData oridata;
    ClusteringInfo oriinfo;
    Document doc("doctest");
    doc.add(1, 1.1);
    doc.add(2, 2.2);
    Document doc2("doctest2");
    doc2.add(1, 2.1);
    doc2.add(2, 3.2);
    oridata.clusteringData.push_back(doc);
    oridata.clusteringData.push_back(doc2);
    std::string catname = "cattest";
    hash_t cathash = Hash_(catname);
    oriinfo.clusteringDocNum = 2;
    oriinfo.clusteringHash = oridata.clusteringHash = cathash;
    oriinfo.clusteringname = catname;
    msgpack::sbuffer sbuf;
    msgpack::pack(&sbuf, oridata);
    msgpack::unpacked msg;
    ClusteringData newdata;
    msgpack::unpack(&msg, sbuf.data(), sbuf.size());
    msg.get().convert(&newdata);
    newdata.output();
}
