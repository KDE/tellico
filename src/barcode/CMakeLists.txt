########### next target ###############

SET(barcode_STAT_SRCS
   barcode.cpp
   barcode_v4l.cpp
   )

add_library(barcode STATIC ${barcode_STAT_SRCS})
