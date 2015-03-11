########### next target ###############

SET(utils_STAT_SRCS
   calendarhandler.cpp
   fieldcomparison.cpp
   iso5426converter.cpp
   iso6937converter.cpp
   isbnvalidator.cpp
   lccnvalidator.cpp
   stringcomparison.cpp
   upcvalidator.cpp
   wallet.cpp
   xmlhandler.cpp
   )

add_library(utils STATIC ${utils_STAT_SRCS})

TARGET_LINK_LIBRARIES(utils
    Qt5::Core
    KF5::Wallet
    KF5::KDELibs4Support
)
