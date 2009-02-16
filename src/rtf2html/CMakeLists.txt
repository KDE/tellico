INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )

########### next target ###############

SET(rtf2html_STAT_SRCS
   fmt_opts.cpp
   rtf2html.cpp
   rtf_keyword.cpp
   rtf_table.cpp
   )

KDE4_ADD_LIBRARY(rtf2html STATIC ${rtf2html_STAT_SRCS})
