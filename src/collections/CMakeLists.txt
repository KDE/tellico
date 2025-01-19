########### next target ###############

set(collections_STAT_SRCS
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
target_link_libraries(collections utils Qt6::Core)
