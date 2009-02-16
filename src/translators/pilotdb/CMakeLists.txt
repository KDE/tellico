ADD_SUBDIRECTORY( libflatfile )
ADD_SUBDIRECTORY( libpalm )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

SET(pilotdb_STAT_SRCS
   pilotdb.cpp
   strop.cpp
   )

KDE4_ADD_LIBRARY(pilotdb STATIC ${pilotdb_STAT_SRCS})
