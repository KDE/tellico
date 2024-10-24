set(images_STAT_SRCS
    image.cpp
    imagedirectory.cpp
    imagefactory.cpp
    imageinfo.cpp
    imagejob.cpp
)

add_library(images STATIC ${images_STAT_SRCS})

target_link_libraries(images
    core
    config
    utils
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::GuiAddons
    Qt${QT_MAJOR_VERSION}::Gui
)

add_dependencies(images tellico_config)
