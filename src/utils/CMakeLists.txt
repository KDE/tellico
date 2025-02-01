########### next target ###############

set(utils_STAT_SRCS
    bibtexhandler.cpp
    cursorsaver.cpp
    datafileregistry.cpp
    gradient.cpp
    guiproxy.cpp
    iso5426converter.cpp
    iso6937converter.cpp
    isbnvalidator.cpp
    lccnvalidator.cpp
    mergeconflictresolver.cpp
    string_utils.cpp
    tellico_utils.cpp
    upcvalidator.cpp
    urlfieldlogic.cpp
    xmlhandler.cpp
)

add_library(utils STATIC ${utils_STAT_SRCS})

target_link_libraries(utils
    collections
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Widgets
    Qt${QT_MAJOR_VERSION}::Xml
    KF${KF_MAJOR_VERSION}::CoreAddons # for KRandom
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::WidgetsAddons
    KF${KF_MAJOR_VERSION}::Codecs
    KF${KF_MAJOR_VERSION}::KIOCore
)
