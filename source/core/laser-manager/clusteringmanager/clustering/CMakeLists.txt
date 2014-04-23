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
)

FILE(GLOB pcaclustering
	"../type/*.cpp"
	"${CMAKE_SOURCE_DIR}/conf/*.cpp"
	"../common/*.cpp"
  	"PCAClustering.cpp"
  	"clustering_main.cpp"
)

ADD_EXECUTABLE(clustering_main ${pcaclustering}
)


TARGET_LINK_LIBRARIES(clustering_main
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)

