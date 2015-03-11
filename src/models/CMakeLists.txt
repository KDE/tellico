########### next target ###############

SET(models_STAT_SRCS
   abstractsortmodel.cpp
   borrowermodel.cpp
   entrygroupmodel.cpp
   entrymodel.cpp
   entryselectionmodel.cpp
   entrysortmodel.cpp
   filtermodel.cpp
   groupsortmodel.cpp
   modeliterator.cpp
   modelmanager.cpp
   modeltest.cpp
   )

add_library(tellicomodels STATIC ${models_STAT_SRCS})

target_link_libraries(tellicomodels
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    KF5::ItemModels
    KF5::KDELibs4Support
)

ADD_DEPENDENCIES(tellicomodels tellico_config)
