# Yaz_FOUND - system has the YAZ library
# Yaz_INCLUDE_DIRS - the YAZ include directory
# Yaz_LIBRARIES - The libraries needed to use YAZ

find_package(PkgConfig)
pkg_check_modules(PC_YAZ yaz)

find_path(Yaz_INCLUDE_DIRS
    NAMES yaz/yaz-version.h
    PATHS ${PC_YAZ_INCLUDEDIR})

find_library(Yaz_LIBRARIES NAMES yaz ${PC_YAZ_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Yaz DEFAULT_MSG Yaz_INCLUDE_DIRS Yaz_LIBRARIES)

mark_as_advanced(Yaz_INCLUDE_DIRS Yaz_LIBRARIES)
