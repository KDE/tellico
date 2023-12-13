SET(images_STAT_SRCS
   image.cpp
   imagedirectory.cpp
   imagefactory.cpp
   imageinfo.cpp
   imagejob.cpp
   )

add_library(images STATIC ${images_STAT_SRCS})

TARGET_LINK_LIBRARIES(images
    core
    config
    utils
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::GuiAddons
    Qt${QT_VERSION_MAJOR}::Gui
)

ADD_DEPENDENCIES(images tellico_config)
