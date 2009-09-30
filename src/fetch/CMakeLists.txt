ADD_SUBDIRECTORY( scripts )

########### next target ###############

SET(fetch_STAT_SRCS
   amazonfetcher.cpp
   amazonrequest.cpp
   animenfofetcher.cpp
   arxivfetcher.cpp
   bibsonomyfetcher.cpp
   citebasefetcher.cpp
   configwidget.cpp
   crossreffetcher.cpp
   discogsfetcher.cpp
   entrezfetcher.cpp
   execexternalfetcher.cpp
   fetcher.cpp
   fetcherjob.cpp
   fetchmanager.cpp
   fetchresult.cpp
   gcstarpluginfetcher.cpp
   googlescholarfetcher.cpp
   hmac_sha2.c
   ibsfetcher.cpp
   imdbfetcher.cpp
   isbndbfetcher.cpp
   messagehandler.cpp
   sha2.c
   srufetcher.cpp
   winecomfetcher.cpp
   yahoofetcher.cpp
   z3950connection.cpp
   z3950fetcher.cpp
   )

KDE4_ADD_LIBRARY(fetch STATIC ${fetch_STAT_SRCS})

########### install files ###############

INSTALL(FILES   z3950-servers.cfg DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
