include(ECMInstallIcons)

set(PIC_FILES
    album.png
    alexandria.png
    amc.png
    bibtex.png
    boardgame.png
    boardgamegeek.png
    book.png
    card.png
    checkmark.png
    cite.png
    coin.png
    collectorz.png
    comic.png
    datacrow.png
    deliciouslibrary.png
    discogs.png
    file.png
    game.png
    gcstar.png
    goodreads.png
    griffith.png
    librarything.png
    nocover_album.png
    nocover_bibtex.png
    nocover_boardgame.png
    nocover_book.png
    nocover_comic.png
    nocover_game.png
    nocover_video.png
    onmyshelf.png
    person-open.png
    person.png
    README.icons
    README.quesnay
    referencer.png
    script.png
    script.svg
    sru.png
    stamp.png
    star_off.png
    star_on.png
    stars10.png
    stars1.png
    stars2.png
    stars3.png
    stars4.png
    stars5.png
    stars6.png
    stars7.png
    stars8.png
    stars9.png
    tellico_mime.svg
    tellico.png
    tellico.svg
    tellico.svg
    video.png
    vinoxml.png
    wine.png
)

install(FILES ${PIC_FILES}
    DESTINATION ${TELLICO_DATA_INSTALL_DIR}/pics
)

set(ICON_FILES
    16-apps-tellico.png
    16-mimetypes-application-x-tellico.png
    22-apps-tellico.png
    22-mimetypes-application-x-tellico.png
    32-apps-tellico.png
    32-mimetypes-application-x-tellico.png
    48-apps-tellico.png
    48-mimetypes-application-x-tellico.png
    64-apps-tellico.png
    64-mimetypes-application-x-tellico.png
    128-apps-tellico.png
    128-mimetypes-application-x-tellico.png
)

ecm_install_icons(ICONS ${ICON_FILES}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
