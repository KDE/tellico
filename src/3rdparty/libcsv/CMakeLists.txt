SET(csv_STAT_SRCS
   libcsv.c
   )

ADD_LIBRARY(csv-tellico STATIC ${csv_STAT_SRCS})
