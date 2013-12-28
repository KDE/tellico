
########### next target ###############

SET(libflatfile_STAT_SRCS
   DB.cpp
   Database.cpp
   )

ADD_LIBRARY(libflatfile-tellico STATIC ${libflatfile_STAT_SRCS})
