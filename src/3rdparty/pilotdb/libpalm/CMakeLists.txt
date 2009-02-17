
########### next target ###############

SET(libpalm_STAT_SRCS
   Database.cpp
   Block.cpp
   )

KDE4_ADD_LIBRARY(libpalm STATIC ${libpalm_STAT_SRCS})
