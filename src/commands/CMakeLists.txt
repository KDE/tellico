
########### next target ###############

SET(commands_STAT_SRCS
   addentries.cpp
   modifyentries.cpp
   removeentries.cpp
   addloans.cpp
   modifyloans.cpp
   removeloans.cpp
   fieldcommand.cpp
   filtercommand.cpp
   reorderfields.cpp
   collectioncommand.cpp
   renamecollection.cpp
   updateentries.cpp
   )

add_library(commands STATIC ${commands_STAT_SRCS})

target_link_libraries(commands
    Qt5::Core
    Qt5::Widgets
    KF5::KDELib4Support
)
