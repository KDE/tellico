IF( WITH_OOO)
ADD_SUBDIRECTORY( ooo )
ENDIF( WITH_OOO)

########### next target ###############

SET(cite_STAT_SRCS
   lyxpipe.cpp
   actionmanager.cpp
   clipboard.cpp
   openoffice.cpp
   )

KDE4_ADD_LIBRARY(cite STATIC ${cite_STAT_SRCS})
