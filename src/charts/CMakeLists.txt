########### next target ###############

set(charts_STAT_SRCS
    barchart.cpp
    chartmanager.cpp
    collectionsizereport.cpp
    groupsummaryreport.cpp
    yeardistributionreport.cpp
)

add_library(charts STATIC ${charts_STAT_SRCS})

target_link_libraries(charts
    tellicomodels
    Qt6::Charts
)
