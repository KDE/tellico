########### next target ###############

SET(utils_STAT_SRCS
   bibtexhandler.cpp
   cursorsaver.cpp
   datafileregistry.cpp
   guiproxy.cpp
   iso5426converter.cpp
   iso6937converter.cpp
   isbnvalidator.cpp
   lccnvalidator.cpp
   string_utils.cpp
   tellico_utils.cpp
   upcvalidator.cpp
   wallet.cpp
   xmlhandler.cpp
   )

add_library(utils STATIC ${utils_STAT_SRCS})

TARGET_LINK_LIBRARIES(utils
    Qt5::Core
    Qt5::Widgets
    Qt5::Xml
    KF5::CoreAddons # for KRandom
    KF5::Wallet
    KF5::I18n
    KF5::WidgetsAddons
    KF5::Codecs
    KF5::KIOCore
)
