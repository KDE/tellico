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

# bibtexcollection depends on utils/bibtexhandler
TARGET_LINK_LIBRARIES(collections utils Qt${QT_MAJOR_VERSION}::Core)
