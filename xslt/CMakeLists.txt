ADD_SUBDIRECTORY( entry-templates )
ADD_SUBDIRECTORY( report-templates )

########### install files ###############

SET(XSLT_FILES
    amazon2tellico.xsl
    arxiv2tellico.xsl
    bluray-logo.png
    crossref2tellico.xsl
    cassette-logo.png
    cd-logo.png
    delicious2tellico.xsl
    dvd-logo.png
    gcstar2tellico.xsl
    hddvd-logo.png
    isbndb2tellico.xsl
    mods2tellico.xsl
    MARC21slim2MODS3.xsl
    MARC21slimUtils.xsl
    pubmed2tellico.xsl
    record-logo.png
    referencer2tellico.xsl
    shadowAlpha.png
    tellico2html.js
    tellico2html.xsl
    tellico2onix.xsl
    tellico-by-author.xsl
    tellico-by-title.xsl
    tellico-common.xsl
    tellico-printing.xsl
    UNIMARC2MODS3.xsl
    vhs-logo.png
    welcome.html
    xmp2tellico.xsl
    yahoo2tellico.xsl
    )

INSTALL(FILES ${XSLT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR} )


