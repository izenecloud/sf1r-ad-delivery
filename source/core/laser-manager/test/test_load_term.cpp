/*
 * test_load_term.cpp
 *
 *  Created on: Apr 3, 2014
 *      Author: alex
 */

#include "type/TermDictionary.h"
#include "common/constant.h"
#include "common/utils.h"
#include <map>
#include <iostream>
using namespace clustering::type;
using namespace clustering;
using namespace std;
int main()
{
    string clustering_path = "/opt/clustering";
    TermDictionary td(clustering_path,ONLY_READ);
    map<hash_t,Term> dic = td.getTermMap();
    for(map<hash_t,Term>::iterator iter = dic.begin(); iter != dic.end(); iter++)
    {
        cout<<"Term hash:"<<iter->first<<" Term index:"<<iter->second.index<<" Term str:"<<iter->second.term_str<<endl;
    }
    return 0;
}

