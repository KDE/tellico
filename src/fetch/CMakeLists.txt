ADD_SUBDIRECTORY( scripts )

########### next target ###############

SET(fetch_STAT_SRCS
   amazonfetcher.cpp
   amazonrequest.cpp
   animenfofetcher.cpp
   arxivfetcher.cpp
   bibliosharefetcher.cpp
   bibsonomyfetcher.cpp
   citebasefetcher.cpp
   configwidget.cpp
   crossreffetcher.cpp
   discogsfetcher.cpp
   doubanfetcher.cpp
   dvdfrfetcher.cpp
   entrezfetcher.cpp
   execexternalfetcher.cpp
   fetcher.cpp
   fetcherinitializer.cpp
   fetcherjob.cpp
   fetchmanager.cpp
   fetchresult.cpp
   filmasterfetcher.cpp
   freebasefetcher.cpp
   gcstarpluginfetcher.cpp
   gcstarthread.cpp
   giantbombfetcher.cpp
   googlebookfetcher.cpp
   googlescholarfetcher.cpp
   hmac_sha2.c
   ibsfetcher.cpp
   imdbfetcher.cpp
   isbndbfetcher.cpp
   messagehandler.cpp
   moviemeterfetcher.cpp
   multifetcher.cpp
   musicbrainzfetcher.cpp
   openlibraryfetcher.cpp
   sha2.c
   srufetcher.cpp
   themoviedbfetcher.cpp
   winecomfetcher.cpp
   xmlfetcher.cpp
   yahoofetcher.cpp
   z3950connection.cpp
   z3950fetcher.cpp
   )

KDE4_ADD_LIBRARY(fetch STATIC ${fetch_STAT_SRCS})

TARGET_LINK_LIBRARIES(fetch
    newstuff
    btparse
    )

# Versions of KDE prior to 4.7 have a character encoding bug in libkxmlrpc
# Use internal version of libkxmlrpc if so
IF (${KDE_VERSION} VERSION_GREATER "4.6.99")
  TARGET_LINK_LIBRARIES(fetch ${KDEPIMLIBS_KXMLRPCCLIENT_LIBS})
ELSE (${KDE_VERSION} VERSION_GREATER "4.6.99")
  ADD_SUBDIRECTORY(xmlrpc)
  TARGET_LINK_LIBRARIES(fetch xmlrpc)
ENDIF (${KDE_VERSION} VERSION_GREATER "4.6.99")

IF( YAZ_FOUND )
  TARGET_LINK_LIBRARIES(fetch ${YAZ_LIBRARIES})
ENDIF( YAZ_FOUND )

IF( QJSON_FOUND )
  TARGET_LINK_LIBRARIES(fetch ${QJSON_LIBRARIES} ${qjson_LIBRARIES})
ENDIF( QJSON_FOUND )

########### install files ###############

INSTALL(FILES   z3950-servers.cfg DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
