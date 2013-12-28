
########### next target ###############

SET(libpalm_STAT_SRCS
   Database.cpp
   Block.cpp
   )

ADD_LIBRARY(libpalm-tellico STATIC ${libpalm_STAT_SRCS})
