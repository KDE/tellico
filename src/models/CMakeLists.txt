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
   groupsortmodel.cpp
   modeliterator.cpp
   modelmanager.cpp
   modeltest.cpp
   )

KDE4_ADD_LIBRARY(tellicomodels STATIC ${models_STAT_SRCS})
