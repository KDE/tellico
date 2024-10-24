if(NOT Btparse_FOUND AND ENABLE_BTPARSE)
    add_subdirectory(btparse)
endif()

if(NOT Csv_FOUND)
    add_subdirectory(libcsv)
endif()

add_subdirectory(rtf2html)
