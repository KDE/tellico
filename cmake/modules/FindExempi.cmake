# LIBEXEMPI_FOUND - system has the LIBEXEMPI library
# LIBEXEMPI_INCLUDE_DIR - the LIBEXEMPI include directory
# LIBEXEMPI_LIBRARIES - The libraries needed to use LIBEXEMPI

#in cache already
if(LIBEXEMPI_INCLUDE_DIR AND LIBEXEMPI_LIBRARIES)
  set(LIBEXEMPI_FIND_QUIETLY TRUE)
endif(LIBEXEMPI_INCLUDE_DIR AND LIBEXEMPI_LIBRARIES)

include(FindPkgConfig)
pkg_check_modules(LIBEXEMPI exempi-2.0)

find_path(LIBEXEMPI_INCLUDE_DIR
    NAMES exempi/xmp.h
    PATH_SUFFIXES exempi-2.0
    PATHS ${LIBEXEMPI_INCLUDEDIR})
find_library(LIBEXEMPI_LIBRARIES NAMES exempi ${LIBEXEMPI_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libexempi DEFAULT_MSG LIBEXEMPI_INCLUDE_DIR LIBEXEMPI_LIBRARIES)

mark_as_advanced(LIBEXEMPI_INCLUDE_DIR LIBEXEMPI_LIBRARIES)
