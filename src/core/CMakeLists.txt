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
    Qt5::Core
    Qt5::Xml
    KF5::I18n
    KF5::JobWidgets
    KF5::KIOWidgets
    )
