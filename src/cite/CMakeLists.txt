SET(cite_STAT_SRCS
   lyxpipe.cpp
   actionmanager.cpp
   clipboard.cpp
   )

add_library(cite STATIC ${cite_STAT_SRCS})

target_link_libraries(cite
    core
    utils
    config
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

ADD_DEPENDENCIES(cite tellico_config)
