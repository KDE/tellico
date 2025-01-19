
########### next target ###############

set(rtf2html_STAT_SRCS
    fmt_opts.cpp
    rtf2html.cpp
    rtf_keyword.cpp
    rtf_table.cpp
)

add_library(rtf2html-tellico STATIC ${rtf2html_STAT_SRCS})

target_link_libraries(rtf2html-tellico
    Qt6::Core
)
