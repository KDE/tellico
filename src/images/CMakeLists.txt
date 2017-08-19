SET(images_STAT_SRCS
   image.cpp
   imagedirectory.cpp
   imagefactory.cpp
   imageinfo.cpp
   )

add_library(images STATIC ${images_STAT_SRCS})

TARGET_LINK_LIBRARIES(images
    core
    config
    utils
    KF5::KIOCore
    KF5::Archive
    KF5::GuiAddons
    Qt5::Gui
)

IF( QIMAGEBLITZ_FOUND )
  TARGET_LINK_LIBRARIES(images ${QIMAGEBLITZ_LIBRARIES})
ENDIF( QIMAGEBLITZ_FOUND )

ADD_DEPENDENCIES(images tellico_config)
