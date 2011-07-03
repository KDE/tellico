########### next target ###############

SET(xmlrpc_STAT_SRCS
  query.cpp
  client.cpp
   )

KDE4_ADD_LIBRARY(xmlrpc STATIC ${xmlrpc_STAT_SRCS})
