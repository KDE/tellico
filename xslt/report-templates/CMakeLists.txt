########### install files ###############

SET(REPORT_FILES
  Column_View.xsl
  Group_Summary.xsl
  Group_View.xsl
  Image_Grid.xsl
  Image_List.xsl
  Loan_View.xsl
  Title_Listing_\(Horizontal\).xsl
  Title_Listing_\(Vertical\).xsl
  Tri-Column.xsl
)

INSTALL(FILES ${REPORT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/report-templates )
