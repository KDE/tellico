ADD_SUBDIRECTORY( libflatfile )
ADD_SUBDIRECTORY( libpalm )

########### next target ###############

SET(pilotdb_STAT_SRCS
    pilotdb.cpp
    strop.cpp
   )

ADD_LIBRARY(pilotdb STATIC ${pilotdb_STAT_SRCS})

TARGET_LINK_LIBRARIES(pilotdb
    libflatfile
    libpalm
    )
