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
    Qt6::Core
    Qt6::Widgets
    Qt6::Xml
    KF6::CoreAddons # for KRandom
    KF6::I18n
    KF6::WidgetsAddons
    KF6::Codecs
    KF6::KIOCore
)
