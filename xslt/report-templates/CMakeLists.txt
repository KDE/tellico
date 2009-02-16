INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )


########### install files ###############

SET(REPORT_FILES
  Column_View.xsl
  Group_Summary.xsl
  Group_View.xsl
  Image_List.xsl
  Loan_View.xsl
  Title_Listing_\(Horizontal\).xsl
  Title_Listing_\(Vertical\).xsl
  Tri-Column.xsl
)

INSTALL(FILES ${REPORT_FILES} DESTINATION ${TELLICO_DATA_INSTALL_DIR}/report-templates )
