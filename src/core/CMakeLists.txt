########### next target ###############

set(core_STAT_SRCS
    filehandler.cpp
    logger.cpp
    netaccess.cpp
    tellico_strings.cpp
)

add_library(core STATIC ${core_STAT_SRCS})

target_link_libraries(core
    utils
    Qt6::Core
    Qt6::Xml
    KF6::I18n
    KF6::JobWidgets
    KF6::KIOWidgets
)
