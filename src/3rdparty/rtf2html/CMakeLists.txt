
########### next target ###############

SET(rtf2html_STAT_SRCS
   fmt_opts.cpp
   rtf2html.cpp
   rtf_keyword.cpp
   rtf_table.cpp
   )

ADD_LIBRARY(rtf2html-tellico STATIC ${rtf2html_STAT_SRCS})

target_link_libraries(rtf2html-tellico
    Qt${QT_MAJOR_VERSION}::Core
)
