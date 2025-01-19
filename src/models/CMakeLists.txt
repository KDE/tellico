########### next target ###############

set(models_STAT_SRCS
    abstractsortmodel.cpp
    borrowermodel.cpp
    entrygroupmodel.cpp
    entryiconmodel.cpp
    entrymodel.cpp
    entryselectionmodel.cpp
    entrysortmodel.cpp
    fieldcomparison.cpp
    filtermodel.cpp
    groupsortmodel.cpp
    modeliterator.cpp
    modelmanager.cpp
    stringcomparison.cpp
)

add_library(tellicomodels STATIC ${models_STAT_SRCS})

target_link_libraries(tellicomodels
    core
    images
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::ItemModels
)

add_dependencies(tellicomodels tellico_config)
