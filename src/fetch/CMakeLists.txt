add_subdirectory(scripts)

########### next target ###############

set(fetch_STAT_SRCS
    adsfetcher.cpp
    amazonfetcher.cpp
    amazonrequest.cpp
    arxivfetcher.cpp
    bedethequefetcher.cpp
    bibliosharefetcher.cpp
    bibsonomyfetcher.cpp
    boardgamegeekfetcher.cpp
    colnectfetcher.cpp
    comicvinefetcher.cpp
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
    fetchrequest.cpp
    fetchresult.cpp
    filmaffinityfetcher.cpp
    filmasterfetcher.cpp
    gaminghistoryfetcher.cpp
    gcstarpluginfetcher.cpp
    gcstarthread.cpp
    giantbombfetcher.cpp
    googlebookfetcher.cpp
    googlescholarfetcher.cpp
    hathitrustfetcher.cpp
    ibsfetcher.cpp
    igdbfetcher.cpp
    imdbfetcher.cpp
    imdbfetcher_query.cpp
    isbndbfetcher.cpp
    itunesfetcher.cpp
    kinofetcher.cpp
    kinopoiskfetcher.cpp
    kinoteatrfetcher.cpp
    messagehandler.cpp
    messagelogger.cpp
    mobygamesfetcher.cpp
    moviemeterfetcher.cpp
    mrlookupfetcher.cpp
    multifetcher.cpp
    musicbrainzfetcher.cpp
    numistafetcher.cpp
    omdbfetcher.cpp
    opdsfetcher.cpp
    openlibraryfetcher.cpp
    rpggeekfetcher.cpp
    springerfetcher.cpp
    srufetcher.cpp
    thegamesdbfetcher.cpp
    themoviedbfetcher.cpp
    thetvdbfetcher.cpp
    tvmazefetcher.cpp
    upcitemdbfetcher.cpp
    vgcollectfetcher.cpp
    videogamegeekfetcher.cpp
    vndbfetcher.cpp
    xmlfetcher.cpp
    z3950connection.cpp
    z3950fetcher.cpp
)

add_library(fetch STATIC ${fetch_STAT_SRCS})

target_link_libraries(fetch
    newstuff
    utils
    ${TELLICO_BTPARSE_LIBS}
    Qt6::Network
    KF6::Archive
    KF6::IconThemes
    KF6::JobWidgets
    KF6::KIOWidgets
    KF6::ConfigWidgets
    KF6::KIOGui
)

if(Yaz_FOUND)
    target_link_libraries(fetch ${Yaz_LIBRARIES})
endif()

########### install files ###############

install(FILES z3950-servers.cfg DESTINATION ${TELLICO_DATA_INSTALL_DIR})
