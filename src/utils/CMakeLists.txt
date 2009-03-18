########### next target ###############

SET(utils_STAT_SRCS
   iso5426converter.cpp
   iso6937converter.cpp
   isbnvalidator.cpp
   lccnvalidator.cpp
   upcvalidator.cpp
   )

KDE4_ADD_LIBRARY(utils STATIC ${utils_STAT_SRCS})
