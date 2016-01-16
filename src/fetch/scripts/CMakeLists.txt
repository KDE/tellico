
########### install files ###############

SET(SCRIPT_FILES
    bedetheque.py
    dark_horse_comics.py
    fr.allocine.py
    ministerio_de_cultura.py
    )

SET(SPEC_FILES
    bedetheque.py.spec
    dark_horse_comics.py.spec
    fr.allocine.py.spec
    ministerio_de_cultura.py.spec
    )

INSTALL(PROGRAMS ${SCRIPT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources )

INSTALL(FILES ${SPEC_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources )


