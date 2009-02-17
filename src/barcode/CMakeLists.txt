########### next target ###############

SET(barcode_STAT_SRCS
   barcode.cpp
   barcode_v4l.cpp
   )

KDE4_ADD_LIBRARY(barcode STATIC ${barcode_STAT_SRCS})
