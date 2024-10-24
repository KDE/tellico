########### install files ###############

set(SCRIPT_FILES
    dark_horse_comics.py
)

set(SPEC_FILES
    dark_horse_comics.py.spec
)

install(PROGRAMS ${SCRIPT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources)

install(FILES ${SPEC_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/data-sources)
