SET(cite_STAT_SRCS
   lyxpipe.cpp
   actionmanager.cpp
   clipboard.cpp
   )

add_library(cite STATIC ${cite_STAT_SRCS})

target_link_libraries(cite
    Qt5::Core
    Qt5::Widgets
)

ADD_DEPENDENCIES(cite tellico_config)
