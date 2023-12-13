########### next target ###############

SET(core_STAT_SRCS
   filehandler.cpp
   logger.cpp
   netaccess.cpp
   tellico_strings.cpp
   )

add_library(core STATIC ${core_STAT_SRCS})

TARGET_LINK_LIBRARIES(core
    utils
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Xml
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::JobWidgets
    KF${KF_MAJOR_VERSION}::KIOWidgets
    )
