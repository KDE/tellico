# CDIO_FOUND - system has libcdio
# CDIO_INCLUDE_DIRS - the libcdio include directory
# CDIO_LIBRARIES - The libcdio libraries

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(CDIO libcdio libiso9660)
  list(APPEND CDIO_INCLUDE_DIRS ${CDIO_libcdio_INCLUDEDIR} ${CDIO_libiso9660_INCLUDEDIR})
  set(CDIO_VERSION ${CDIO_libcdio_VERSION})
endif()
if(NOT CDIO_FOUND)
  find_path(CDIO_INCLUDE_DIRS cdio/cdio.h)
  find_library(CDIO_LIBRARIES NAMES cdio)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CDIO FOUND_VAR CDIO_FOUND REQUIRED_VARS CDIO_INCLUDE_DIRS CDIO_LIBRARIES VERSION_VAR CDIO_VERSION)

mark_as_advanced(CDIO_INCLUDE_DIRS CDIO_LIBRARIES)
