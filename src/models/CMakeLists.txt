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
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Widgets
    KF${KF_MAJOR_VERSION}::ItemModels
)

add_dependencies(tellicomodels tellico_config)
