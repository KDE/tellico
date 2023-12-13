########### next target ###############

SET(charts_STAT_SRCS
   barchart.cpp
   chartmanager.cpp
   collectionsizereport.cpp
   groupsummaryreport.cpp
   yeardistributionreport.cpp
)

add_library(charts STATIC ${charts_STAT_SRCS})

target_link_libraries(charts
    tellicomodels
    Qt${QT_VERSION_MAJOR}::Charts
)
