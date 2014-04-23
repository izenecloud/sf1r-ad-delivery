INCLUDE_DIRECTORIES (
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${Protobuf_INCLUDE_DIRS}
  ${Memcached_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${CLUSTINGLIB_PATH}
  ${PREDICTLIB_PATH}
)

FILE(GLOB test_load_term
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
  	 "test_load_term.cpp"
)

ADD_EXECUTABLE(test_load_term ${test_load_term}
)


TARGET_LINK_LIBRARIES(test_load_term
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_rpc_server
	"${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${SERVICELIB_PATH}/CLUSTERINGServerRequest.cpp"
  	 "test_rpc_server.cpp"
)

ADD_EXECUTABLE(test_rpc_server ${test_rpc_server}
)


TARGET_LINK_LIBRARIES(test_rpc_server
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
   pthread
)


FILE(GLOB test_level_db_for_res
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${CLUSTINGLIB_PATH}/type/*.cpp"
  	 "test_level_db_for_res.cpp"
)

ADD_EXECUTABLE(test_level_db_for_res ${test_level_db_for_res}
)


TARGET_LINK_LIBRARIES(test_level_db_for_res
   ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
 ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_message_pack
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${CLUSTINGLIB_PATH}/type/*.cpp"
  	 "test_message_pack.cpp"
)

ADD_EXECUTABLE(test_message_pack ${test_message_pack}
)


TARGET_LINK_LIBRARIES(test_message_pack
   ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
 ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_level_db
  	 "test_level_db.cpp"
)

ADD_EXECUTABLE(test_level_db ${test_level_db}
)


TARGET_LINK_LIBRARIES(test_level_db
  ${Boost_LIBRARIES}
  ${Glog_LIBRARIES}
 ${Boost_THREAD_LIBRARY}
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_serialization
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${CLUSTINGLIB_PATH}/type/*.cpp"
	 "${PREDICTLIB_PATH}/TopNClusterContainer.cpp"
  	 "test_serialization.cpp"
)

ADD_EXECUTABLE(test_serialization ${test_serialization}
)


TARGET_LINK_LIBRARIES(test_serialization
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_topnclustercontainer
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${CLUSTINGLIB_PATH}/type/*.cpp"
	 "${PREDICTLIB_PATH}/*.cpp"
  	 "test_topnclustercontainer.cpp"
)

ADD_EXECUTABLE(test_topnclustercontainer ${test_topnclustercontainer}
)


TARGET_LINK_LIBRARIES(test_topnclustercontainer
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_laseronlinemodel
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${CLUSTINGLIB_PATH}/type/*.cpp"
	 "${PREDICTLIB_PATH}/*.cpp"
  	 "test_laseronlinemodel.cpp"
)

ADD_EXECUTABLE(test_laseronlinemodel ${test_laseronlinemodel}
)


TARGET_LINK_LIBRARIES(test_laseronlinemodel
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

FILE(GLOB test_predict
	 "${CMAKE_SOURCE_DIR}/conf/*.cpp"
	 "${CLUSTINGLIB_PATH}/type/*.cpp"
	 "${PREDICTLIB_PATH}/*.cpp"
  	 "test_predict.cpp"
)

ADD_EXECUTABLE(test_predict ${test_predict}
)


TARGET_LINK_LIBRARIES(test_predict
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

