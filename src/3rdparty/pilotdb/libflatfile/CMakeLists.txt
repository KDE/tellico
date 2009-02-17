
########### next target ###############

SET(libflatfile_STAT_SRCS
   DB.cpp
   Database.cpp
   )

KDE4_ADD_LIBRARY(libflatfile STATIC ${libflatfile_STAT_SRCS})
