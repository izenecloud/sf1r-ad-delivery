SF1R-Ad-Delivery
=======================================
*Advertising delivery engine based on SF1R.*

### Features
* _Sponsored Search_. Automatic bid optimization is provided with the semantic of `broad match`. Currently, 
the `broad match` is delivered through `succinct self index`, which is the fastest top-K union index in the world. However, it's an overkill to use `succinct self index` for such purpose, we plan to use [Roaring Bitmap](https://github.com/izenecloud/izenelib/blob/master/include/am/bitmap/RoaringBitmap.h) in future which is the fastest compressed bitmap delivered by [Daniel Lemire](https://github.com/lemire). 

* _Online engine for TARE_. `TARE` refers to `Targeted Advertising and Recommender Engine`, which aims to build
an unified engine for both targeted advertising and recommendation. Currently, `TARE` is tailored for products only.

![Topology](https://raw.githubusercontent.com/izenecloud/sf1r/master/docs/source/images/sf1r.png)


### Dependencies
The dependencies are similar with that of [SF1R](https://github.com/izenecloud/sf1r-lite). Besides, these third party repositores are required:

* __[avro](https://github.com/izenecloud/thirdparty/tree/master/avro)__: The C++ serialization for [AVRO](http://avro.apache.or).


* __[libcuchbase](https://github.com/izenecloud/thirdparty/tree/master/libcouchbase)__: The C++ client for Couchbase.



### License
The SF1R project is published under the Apache License, Version 2.0:
http://www.apache.org/licenses/LICENSE-2.0
