INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
LINK_DIRECTORIES( ${CMAKE_BINARY_DIR} )

########### next target ###############

SET(commands_STAT_SRCS
   addentries.cpp
   modifyentries.cpp
   removeentries.cpp
   addloans.cpp
   modifyloans.cpp
   removeloans.cpp
   fieldcommand.cpp
   filtercommand.cpp
   reorderfields.cpp
   collectioncommand.cpp
   renamecollection.cpp
   updateentries.cpp
   )

KDE4_ADD_LIBRARY(commands STATIC ${commands_STAT_SRCS})
