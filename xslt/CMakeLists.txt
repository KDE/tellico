ADD_SUBDIRECTORY( entry-templates ) 
ADD_SUBDIRECTORY( report-templates ) 

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### install files ###############

INSTALL(FILES   tellico-printing.xsl tellico-by-author.xsl tellico-by-title.xsl          tellico2html.xsl shadowAlpha.png cassette-logo.png cd-logo.png          dvd-logo.png record-logo.png vhs-logo.png tellico-common.xsl          mods2tellico.xsl amazon2tellico.xsl MARC21slim2MODS3.xsl          MARC21slimUtils.xsl pubmed2tellico.xsl tellico2onix.xsl          UNIMARC2MODS3.xsl tellico2html.js yahoo2tellico.xsl          isbndb2tellico.xsl bluray-logo.png hddvd-logo.png gcstar2tellico.xsl          xmp2tellico.xsl crossref2tellico.xsl arxiv2tellico.xsl          referencer2tellico.xsl delicious2tellico.xsl welcome.html DESTINATION ${TELLICO_DATA_INSTALL_DIR} )


