SET(csv_STAT_SRCS
   libcsv.c
   )

ADD_LIBRARY(csv STATIC ${csv_STAT_SRCS})
