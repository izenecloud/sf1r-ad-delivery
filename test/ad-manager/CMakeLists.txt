INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs

  sf1r_mining_manager
  sf1r_query_manager
  sf1r_search_manager
  sf1r_document_manager
  sf1r_ranking_manager
  sf1r_la_manager
  sf1r_ad_manager
  sf1r_laser_manager
  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_aggregator_manager
  sf1r_node_manager
  sf1r_bundle_index
  sf1r_bundle_mining


  sf1r_log_manager
  sf1r_la_manager
  sf1r_directory_manager
  sf1r_ad_manager
  sf1r_common
  sf1r_net

  ${ilplib_LIBRARIES}
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${izenelib_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${XML2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  ${AVRO_LIBRARIES}
  ${LIBCOUCHBASE_LIBRARIES}
  )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )
  #ADD_EXECUTABLE(t_split_ustr
  #  Runner.cpp
  #  t_split_ustr.cpp
  #)
  #TARGET_LINK_LIBRARIES(t_split_ustr ${libs})
  #SET_TARGET_PROPERTIES(t_split_ustr PROPERTIES
  #  RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
  #  )
  #ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_split_ustr")

  ADD_EXECUTABLE(t_sponsored_ad
    t_sponsored_ad.cpp
  )
  TARGET_LINK_LIBRARIES(t_sponsored_ad ${libs})
  SET_TARGET_PROPERTIES(t_sponsored_ad PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  #ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_ad_ctr")

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

