
KDE4_CREATE_HANDBOOK( index.docbook
  INSTALL_DESTINATION ${HTML_INSTALL_DIR}/en
  SUBDIR tellico
  )

# the macro automatically installs all *.docbook and images

set(DOC_EXTRA_FILES
    freebsd-doc-license.html)

INSTALL(FILES ${DOC_EXTRA_FILES} DESTINATION ${HTML_INSTALL_DIR}/en/tellico )
