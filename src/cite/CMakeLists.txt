SET(cite_STAT_SRCS
   lyxpipe.cpp
   actionmanager.cpp
   clipboard.cpp
   )

KDE4_ADD_LIBRARY(cite STATIC ${cite_STAT_SRCS})
