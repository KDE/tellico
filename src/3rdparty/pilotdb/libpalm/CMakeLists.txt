
########### next target ###############

SET(libpalm_STAT_SRCS
   Database.cpp
   Block.cpp
   )

ADD_LIBRARY(libpalm STATIC ${libpalm_STAT_SRCS})
