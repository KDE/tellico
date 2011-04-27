ADD_SUBDIRECTORY( entry-templates )
ADD_SUBDIRECTORY( report-templates )

########### install files ###############

SET(XSLT_FILES
    amazon2tellico.xsl
    arxiv2tellico.xsl
    biblioshare2tellico.xsl
    bluray-logo.png
    cassette-logo.png
    cd-logo.png
    crossref2tellico.xsl
    delicious2tellico.xsl
    discogs2tellico.xsl
    douban2tellico.xsl
    dvd-logo.png
    dvdfr2tellico.xsl
    gcstar2tellico.xsl
    giantbomb2tellico.xsl
    hddvd-logo.png
    isbndb2tellico.xsl
    MARC21slim2MODS3.xsl
    MARC21slimUtils.xsl
    mods2tellico.xsl
    musicbrainz2tellico.xsl
    pubmed2tellico.xsl
    record-logo.png
    referencer2tellico.xsl
    shadowAlpha.png
    srw2tellico.xsl
    tellico2gcstar.xsl
    tellico2html.js
    tellico2html.xsl
    tellico2onix.xsl
    tellico-by-author.xsl
    tellico-by-title.xsl
    tellico-common.xsl
    tellico-printing.xsl
    tmdb2tellico.xsl
    UNIMARC2MODS3.xsl
    vhs-logo.png
    welcome.html
    winecom2tellico.xsl
    xmp2tellico.xsl
    yahoo2tellico.xsl
    )

INSTALL(FILES ${XSLT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR} )


