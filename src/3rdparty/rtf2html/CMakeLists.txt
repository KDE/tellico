
########### next target ###############

SET(rtf2html_STAT_SRCS
   fmt_opts.cpp
   rtf2html.cpp
   rtf_keyword.cpp
   rtf_table.cpp
   )

KDE4_ADD_LIBRARY(rtf2html STATIC ${rtf2html_STAT_SRCS})
