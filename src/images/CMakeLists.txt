SET(images_STAT_SRCS
   image.cpp
   imagedirectory.cpp
   imagefactory.cpp
   imageinfo.cpp
   )

KDE4_ADD_LIBRARY(images STATIC ${images_STAT_SRCS})
# the following linking seems to be necessary for the KDE build dashboard to succeed
TARGET_LINK_LIBRARIES(images
    ${QIMAGEBLITZ_LIBRARIES}
    ${KDE4_KIO_LIBS}
)

ADD_DEPENDENCIES(images tellico_config)
