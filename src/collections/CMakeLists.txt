INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

SET(collections_STAT_SRCS
   winecollection.cpp
   stampcollection.cpp
   comicbookcollection.cpp
   cardcollection.cpp
   coincollection.cpp
   bibtexcollection.cpp
   musiccollection.cpp
   videocollection.cpp
   bookcollection.cpp
   gamecollection.cpp
   filecatalog.cpp
   boardgamecollection.cpp
   )

KDE4_ADD_LIBRARY(collections STATIC ${collections_STAT_SRCS})
