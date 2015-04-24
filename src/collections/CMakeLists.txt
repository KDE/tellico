
########### next target ###############

SET(collections_STAT_SRCS
   bibtexcollection.cpp
   boardgamecollection.cpp
   bookcollection.cpp
   cardcollection.cpp
   coincollection.cpp
   collectioninitializer.cpp
   comicbookcollection.cpp
   filecatalog.cpp
   gamecollection.cpp
   musiccollection.cpp
   stampcollection.cpp
   videocollection.cpp
   winecollection.cpp
   )

add_library(collections STATIC ${collections_STAT_SRCS})

# bibtexcollection depends on translators/bibtexhandler
# they all depend on ksharedptr in KDELibs4Support for the moment
TARGET_LINK_LIBRARIES(collections translators Qt5::Core KF5::KDELibs4Support)
