set(cite_STAT_SRCS
    lyxpipe.cpp
    actionmanager.cpp
    clipboard.cpp
)

add_library(cite STATIC ${cite_STAT_SRCS})

target_link_libraries(cite
    core
    utils
    config
    Qt6::Core
    Qt6::Widgets
)

add_dependencies(cite tellico_config)
