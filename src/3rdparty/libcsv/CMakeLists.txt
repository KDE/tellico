SET(csv_STAT_SRCS
   libcsv.c
   )

KDE4_ADD_LIBRARY(csv STATIC ${csv_STAT_SRCS})
