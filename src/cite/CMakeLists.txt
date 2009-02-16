IF( WITH_OOO)
ADD_SUBDIRECTORY( ooo )
ENDIF( WITH_OOO)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

SET(cite_STAT_SRCS
   lyxpipe.cpp
   actionmanager.cpp
   clipboard.cpp
   openoffice.cpp
   )

KDE4_ADD_LIBRARY(cite STATIC ${cite_STAT_SRCS})
