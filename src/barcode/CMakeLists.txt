########### next target ###############

set(barcode_STAT_SRCS
    barcode.cpp
    barcode_v4l.cpp
)

add_library(barcode STATIC ${barcode_STAT_SRCS})

target_link_libraries(barcode
    Qt6::Core
    Qt6::Gui
)
