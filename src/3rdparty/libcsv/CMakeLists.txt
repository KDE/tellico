set(csv_STAT_SRCS
    libcsv.c
)

add_library(csv-tellico STATIC ${csv_STAT_SRCS})
