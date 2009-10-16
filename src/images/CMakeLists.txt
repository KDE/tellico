SET(images_STAT_SRCS
   image.cpp
   imagedirectory.cpp
   imagefactory.cpp
   imageinfo.cpp
   )

KDE4_ADD_LIBRARY(images STATIC ${images_STAT_SRCS})
TARGET_LINK_LIBRARIES(images ${QIMAGEBLITZ_LIBRARIES})

ADD_DEPENDENCIES(images tellico_config)
