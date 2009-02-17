########### next target ###############

SET(models_STAT_SRCS
   abstractentrymodel.cpp
   abstractsortmodel.cpp
   borrowermodel.cpp
   entrygroupmodel.cpp
   entrymodel.cpp
   entrysortmodel.cpp
   entrytitlemodel.cpp
   filtermodel.cpp
   modeltest.cpp
   )

KDE4_ADD_LIBRARY(models STATIC ${models_STAT_SRCS})
