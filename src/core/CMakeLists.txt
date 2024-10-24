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
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Xml
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::JobWidgets
    KF${KF_MAJOR_VERSION}::KIOWidgets
)
