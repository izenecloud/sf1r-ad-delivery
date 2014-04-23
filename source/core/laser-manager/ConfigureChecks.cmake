##################################################
# Headers
#####
INCLUDE(CheckIncludeFile)
##################################################
# Our Proprietary Libraries
#####


SET( USE_IZENECMA TRUE )
SET( USE_WISEKMA TRUE)
SET( USE_IZENEJMA TRUE )

FIND_PACKAGE(izenelib REQUIRED COMPONENTS
  febird
  jemalloc
  json
  am
  msgpack
 # snappy
  compressor
  leveldb
  izene_util
  re2
  )
FIND_PACKAGE(ilplib REQUIRED)
##################################################
# Other Libraries
#####

FIND_PACKAGE(Threads REQUIRED)

SET(Boost_ADDITIONAL_VERSIONS ${Boost_FIND_VERSION})
FIND_PACKAGE(Boost ${Boost_FIND_VERSION} REQUIRED
  COMPONENTS
  system
  program_options
  thread
  regex
  date_time
  serialization
  filesystem
  unit_test_framework
  iostreams
  )

FIND_PACKAGE(Glog REQUIRED)
#FIND_PACKAGE(Protobuf REQUIRED)
#FIND_PACKAGE(Memcached REQUIRED)
##################################################
# Doxygen
#####
#FIND_PACKAGE(Doxygen)
#IF(DOXYGEN_DOT_EXECUTABLE)
#  OPTION(USE_DOT "use dot in doxygen?" FLASE)
#ENDIF(DOXYGEN_DOT_EXECUTABLE)

add_definitions("-fpermissive")

IF( USE_WISEKMA )
  FIND_PACKAGE(wisekma REQUIRED)
  ADD_DEFINITIONS( -DUSE_WISEKMA=TRUE )
ELSE( USE_WISEKMA )
  SET( wisekma_LIBRARIES "" )
  SET( wisekma_INCLUDE_DIRS "" )
  SET( wisekma_LIBRARY_DIRS "" )
ENDIF( USE_WISEKMA )

IF( USE_IZENECMA )
  FIND_PACKAGE(izenecma REQUIRED)
  ADD_DEFINITIONS( -DUSE_IZENECMA=TRUE )
ELSE( USE_IZENECMA )
  SET( izenecma_INCLUDE_DIRS "" )
  SET( izenecma_LIBRARIES "" )
  SET( izenecma_LIBRARY_DIRS "" )
ENDIF( USE_IZENECMA )

IF( USE_IZENEJMA )
  FIND_PACKAGE(izenejma REQUIRED)
  ADD_DEFINITIONS( -DUSE_IZENEJMA=TRUE )
ELSE( USE_IZENEJMA )
  SET( izenejma_INCLUDE_DIRS "" )
  SET( izenejma_LIBRARIES "" )
  SET( izenejma_LIBRARY_DIRS "" )
ENDIF( USE_IZENEJMA )

FIND_PACKAGE(TokyoCabinet 1.4.29 REQUIRED)
IF(TokyoCabinet_FOUND)
    MESSAGE(STATUS "TokyoCabine:")
    MESSAGE(STATUS "  header: ${TokyoCabinet_INCLUDE_DIRS}")
    MESSAGE(STATUS "  version: ${TokyoCabinet_VERSION}")
    include_directories(${TokyoCabinet_INCLUDE_DIRS})
ELSE(TokyoCabinet_FOUND)
    MESSAGE(STATUS "TokyoCabinet: not found")
ENDIF(TokyoCabinet_FOUND)

SET(USE_DOT_YESNO NO)
IF(USE_DOT)
  SET(USE_DOT_YESNO YES)
ENDIF(USE_DOT)

set(SYS_LIBS
  m rt dl z
)

