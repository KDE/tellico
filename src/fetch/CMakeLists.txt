ADD_SUBDIRECTORY( scripts )

########### next target ###############

SET(fetch_STAT_SRCS
   allocinefetcher.cpp
   amazonfetcher.cpp
   amazonrequest.cpp
   animenfofetcher.cpp
   arxivfetcher.cpp
   bedethequefetcher.cpp
   bibliosharefetcher.cpp
   bibsonomyfetcher.cpp
   boardgamegeekfetcher.cpp
   configwidget.cpp
   crossreffetcher.cpp
   dbcfetcher.cpp
   dblpfetcher.cpp
   discogsfetcher.cpp
   doubanfetcher.cpp
   dvdfrfetcher.cpp
   entrezfetcher.cpp
   execexternalfetcher.cpp
   fetcher.cpp
   fetcherinfolistitem.cpp
   fetcherinitializer.cpp
   fetcherjob.cpp
   fetchmanager.cpp
   fetchresult.cpp
   filmasterfetcher.cpp
   gcstarpluginfetcher.cpp
   gcstarthread.cpp
   giantbombfetcher.cpp
   googlebookfetcher.cpp
   googlescholarfetcher.cpp
   hathitrustfetcher.cpp
   hmac_sha2.c
   ibsfetcher.cpp
   igdbfetcher.cpp
   imdbfetcher.cpp
   isbndbfetcher.cpp
   kinofetcher.cpp
   kinopoiskfetcher.cpp
   messagehandler.cpp
   moviemeterfetcher.cpp
   mrlookupfetcher.cpp
   multifetcher.cpp
   musicbrainzfetcher.cpp
   omdbfetcher.cpp
   openlibraryfetcher.cpp
   sha2.c
   springerfetcher.cpp
   srufetcher.cpp
   thegamesdbfetcher.cpp
   themoviedbfetcher.cpp
   videogamegeekfetcher.cpp
   vndbfetcher.cpp
   xmlfetcher.cpp
   z3950connection.cpp
   z3950fetcher.cpp
   )

add_library(fetch STATIC ${fetch_STAT_SRCS})

TARGET_LINK_LIBRARIES(fetch
    newstuff
    utils
    ${TELLICO_BTPARSE_LIBS}
    Qt5::Network
    KF5::Archive
    KF5::IconThemes
    KF5::JobWidgets
    KF5::KIOWidgets
    )

IF(${KF5KIO_VERSION} VERSION_GREATER "5.18.0")
  TARGET_LINK_LIBRARIES(fetch
      KF5::KIOGui
      )
ENDIF(${KF5KIO_VERSION} VERSION_GREATER "5.18.0")

IF( Yaz_FOUND )
  TARGET_LINK_LIBRARIES(fetch ${Yaz_LIBRARIES})
ENDIF( Yaz_FOUND )

########### install files ###############

INSTALL(FILES z3950-servers.cfg DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
