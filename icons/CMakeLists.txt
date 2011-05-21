########### install files ###############

set(ICON_FILES
    album.png
    alexandria.png
    amc.png
    bibtex.png
    boardgame.png
    book.png
    card.png
    checkmark.png
    cite.png
    coin.png
    comic.png
    deliciouslibrary.png
    file.png
    game.png
    gcstar.png
    griffith.png
    goodreads.png
    nocover_album.png
    nocover_bibtex.png
    nocover_boardgame.png
    nocover_book.png
    nocover_comic.png
    nocover_game.png
    nocover_video.png
    palm.png
    person-open.png
    person.png
    README.icons
    README.quesnay
    referencer.png
    script.png
    script.svg
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
    wine.png
    )

INSTALL(FILES ${ICON_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/pics )

KDE4_INSTALL_ICONS( ${ICON_INSTALL_DIR} )


