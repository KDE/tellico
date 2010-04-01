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
   fetcherinitializer.cpp
   fetcherjob.cpp
   fetchmanager.cpp
   fetchresult.cpp
   gcstarpluginfetcher.cpp
   giantbombfetcher.cpp
   googlescholarfetcher.cpp
   hmac_sha2.c
   ibsfetcher.cpp
   imdbfetcher.cpp
   isbndbfetcher.cpp
   messagehandler.cpp
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
IF( YAZ_FOUND )
  TARGET_LINK_LIBRARIES(fetch ${YAZ_LIBRARIES})
ENDIF( YAZ_FOUND )

IF( QJSON_FOUND )
  TARGET_LINK_LIBRARIES(fetch ${QJSON_LIBRARIES})
ENDIF( QJSON_FOUND )

########### install files ###############

INSTALL(FILES   z3950-servers.cfg DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
