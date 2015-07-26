########### next target ###############

SET(barcode_STAT_SRCS
   barcode.cpp
   barcode_v4l.cpp
   )

add_library(barcode STATIC ${barcode_STAT_SRCS})

TARGET_LINK_LIBRARIES(barcode
    Qt5::Core
    Qt5::Gui
)
