INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### install files ###############

INSTALL(FILES   Default.xsl Video.xsl Album.xsl Fancy.xsl Compact.xsl DESTINATION ${TELLICO_DATA_INSTALL_DIR}/entry-templates )
