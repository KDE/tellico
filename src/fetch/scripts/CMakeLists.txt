INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )

########### install files ###############

INSTALL(PROGRAMS   fr.allocine.py  ministerio_de_cultura.py  dark_horse_comics.py  boardgamegeek.rb DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources )

INSTALL(FILES   fr.allocine.py.spec  ministerio_de_cultura.py.spec  dark_horse_comics.py.spec  boardgamegeek.rb.spec DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources )


