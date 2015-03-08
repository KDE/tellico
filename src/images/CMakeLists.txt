SET(images_STAT_SRCS
   image.cpp
   imagedirectory.cpp
   imagefactory.cpp
   imageinfo.cpp
   )

add_library(images STATIC ${images_STAT_SRCS})
TARGET_LINK_LIBRARIES(images
    core
    gui
)
# the following linking seems to be necessary for the KDE build dashboard to succeed
TARGET_LINK_LIBRARIES(images
    ${QIMAGEBLITZ_LIBRARIES}
    KF5::KIOCore
)

ADD_DEPENDENCIES(images tellico_config)
