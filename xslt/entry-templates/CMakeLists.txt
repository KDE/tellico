########### install files ###############

set(ENTRY_FILES
    Album.xsl
    Compact.xsl
    Default.xsl
    Fancy.xsl
    Video.xsl
)

install(FILES ${ENTRY_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/entry-templates)
