INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

SET(libpalm_STAT_SRCS
   Database.cpp
   Block.cpp
   )

KDE4_ADD_LIBRARY(libpalm STATIC ${libpalm_STAT_SRCS})
