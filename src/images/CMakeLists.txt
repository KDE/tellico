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
    KF6::KIOCore
    KF6::Archive
    KF6::GuiAddons
    Qt6::Gui
)

add_dependencies(images tellico_config)
