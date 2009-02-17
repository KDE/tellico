
########### install files ###############

SET(SCRIPT_FILES
    fr.allocine.py
    ministerio_de_cultura.py
    dark_horse_comics.py
    boardgamegeek.rb
    )

SET(SPEC_FILES
    fr.allocine.py.spec
    ministerio_de_cultura.py.spec
    dark_horse_comics.py.spec
    boardgamegeek.rb.spec
    )

INSTALL(PROGRAMS ${SCRIPT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources )

INSTALL(FILES ${SPEC_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources )


