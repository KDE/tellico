SET(images_STAT_SRCS
   image.cpp
   imagedirectory.cpp
   imagefactory.cpp
   imageinfo.cpp
   )

KDE4_ADD_LIBRARY(images STATIC ${images_STAT_SRCS})
TARGET_LINK_LIBRARIES(images
    core
    gui
)
# the following linking seems to be necessary for the KDE build dashboard to succeed
TARGET_LINK_LIBRARIES(images
    ${KDE4_KIO_LIBS}
)

IF( QIMAGEBLITZ_FOUND )
  TARGET_LINK_LIBRARIES(images ${QIMAGEBLITZ_LIBRARIES})
ENDIF( QIMAGEBLITZ_FOUND )

ADD_DEPENDENCIES(images tellico_config)
