INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### install files ###############

INSTALL(FILES   album.png  bibtex.png  tellico.png  tellico.svg  tellico.svg  tellico_mime.svg  boardgame.png  book.png  card.png  coin.png  comic.png  game.png  person-open.png  person.png  stamp.png  star_on.png  star_off.png  stars1.png  stars2.png  stars3.png  stars4.png  stars5.png  stars6.png  stars7.png  stars8.png  stars9.png  stars10.png  video.png  wine.png  checkmark.png  README.icons  README.quesnay  file.png  nocover_album.png  nocover_boardgame.png  nocover_book.png  nocover_bibtex.png  nocover_comic.png  nocover_game.png  nocover_video.png  script.svg  script.png DESTINATION ${TELLICO_DATA_INSTALL_DIR}/pics )

KDE4_INSTALL_ICONS( ${ICON_INSTALL_DIR} )


