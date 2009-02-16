ADD_SUBDIRECTORY( scripts )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

########### next target ###############

SET(fetch_STAT_SRCS
   amazonfetcher.cpp
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
   fetchmanager.cpp
   gcstarpluginfetcher.cpp
   googlescholarfetcher.cpp
   ibsfetcher.cpp
   imdbfetcher.cpp
   isbndbfetcher.cpp
   messagehandler.cpp
   srufetcher.cpp
   yahoofetcher.cpp
   z3950connection.cpp
   z3950fetcher.cpp
   )

KDE4_ADD_LIBRARY(fetch STATIC ${fetch_STAT_SRCS})

########### install files ###############

INSTALL(FILES   z3950-servers.cfg DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
