
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
  ${izenejma_INCLUDE_DIRS}
  ${CLUSTINGLIB_PATH}
  )

FILE(GLOB laser_SRC
    "LaserProcess.cpp"
    "LaserServer.cpp"
    "RouterInitializer.cpp"
    "OnSignal.cpp"
    "RpcServer.cpp"
    "${PREDICTLIB_PATH}/*.cpp"
    "TermParser.cpp"
    "CLUSTERINGServerRequest.cpp"
	"${CMAKE_SOURCE_DIR}/conf/*.cpp"
	"${CLUSTINGLIB_PATH}/type/*.cpp"
)

ADD_EXECUTABLE(laser ${laser_SRC}
)

TARGET_LINK_LIBRARIES(laser
   ${Boost_SYSTEM_LIBRARY}
   ${LIBS}
)
