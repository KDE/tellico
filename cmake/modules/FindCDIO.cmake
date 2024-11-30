# CDIO_FOUND - system has libcdio
# CDIO_INCLUDE_DIRS - the libcdio include directory
# CDIO_LIBRARIES - The libcdio libraries

find_package(PkgConfig)
pkg_check_modules(PC_CDIO libcdio libiso9660)
list(APPEND PC_CDIO_INCLUDE_DIRS ${PC_CDIO_libcdio_INCLUDEDIR} ${PC_CDIO_libiso9660_INCLUDEDIR})

find_path(CDIO_INCLUDE_DIRS cdio/cdio.h PATHS ${PC_CDIO_INCLUDE_DIRS})
find_library(CDIO_LIBRARIES NAMES cdio libiso9660 ${PC_CDIO_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CDIO FOUND_VAR CDIO_FOUND REQUIRED_VARS CDIO_INCLUDE_DIRS CDIO_LIBRARIES)

mark_as_advanced(CDIO_INCLUDE_DIRS CDIO_LIBRARIES)
