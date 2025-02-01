add_subdirectory(entry-templates)
add_subdirectory(report-templates)

########### install files ###############

set(XSLT_FILES
    arxiv2tellico.xsl
    atom2tellico.xsl
    biblioshare2tellico.xsl
    bibtexml2tellico.xsl
    bluray-logo.png
    boardgamegeek2tellico.xsl
    cassette-logo.png
    collectorz2tellico.xsl
    comicvine2tellico.xsl
    cd-logo.png
    crossref2tellico.xsl
    datacrow2tellico.xsl
    dbc2tellico.xsl
    dblp2tellico.xsl
    delicious2tellico.xsl
    dvd-logo.png
    dvdfr2tellico.xsl
    gcstar2tellico.xsl
    giantbomb2tellico.xsl
    goodreads2tellico.xsl
    griffith2tellico.xsl
    hddvd-logo.png
    isfdb2tellico.xsl
    MARC21slim2MODS3.xsl
    MARC21slimUtils.xsl
    mods2tellico.xsl
    musicbrainz2tellico.xsl
    pubmed2tellico.xsl
    record-logo.png
    referencer2tellico.xsl
    shadowAlpha.png
    springer2tellico.xsl
    srw2tellico.xsl
    tellico2gcstar.xsl
    tellico2html.js
    tellico2html.xsl
    tellico2onix.xsl
    tellico-by-author.xsl
    tellico-by-title.xsl
    tellico-common.xsl
    tellico-printing.xsl
    UNIMARC2MODS3.xsl
    unixref2tellico.xsl
    vhs-logo.png
    vinoxml2tellico.xsl
    welcome.html
    xmp2tellico.xsl
)

install(FILES ${XSLT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR})
