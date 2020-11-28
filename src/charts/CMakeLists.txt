########### next target ###############

SET(charts_STAT_SRCS
    groupsummary.cpp
   )

add_library(charts STATIC ${charts_STAT_SRCS})

TARGET_LINK_LIBRARIES(charts
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Charts
    KF5::I18n
)
