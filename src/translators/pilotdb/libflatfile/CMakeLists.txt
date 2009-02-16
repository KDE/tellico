INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

SET(libflatfile_STAT_SRCS
   DB.cpp
   Database.cpp
   )

KDE4_ADD_LIBRARY(libflatfile STATIC ${libflatfile_STAT_SRCS})
