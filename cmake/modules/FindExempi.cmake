# Exempi_FOUND - system has the LIBEXEMPI library
# Exempi_INCLUDE_DIRS - the LIBEXEMPI include directory
# Exempi_LIBRARIES - The libraries needed to use LIBEXEMPI

find_package(PkgConfig)
pkg_check_modules(PC_LIBEXEMPI exempi-2.0)

find_path(Exempi_INCLUDE_DIRS
    NAMES exempi/xmp.h
    PATH_SUFFIXES exempi-2.0
    PATHS ${PC_LIBEXEMPI_INCLUDEDIR})
find_library(Exempi_LIBRARIES NAMES exempi ${PC_LIBEXEMPI_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Exempi DEFAULT_MSG Exempi_INCLUDE_DIRS Exempi_LIBRARIES)

mark_as_advanced(Exempi_INCLUDE_DIRS Exempi_LIBRARIES)
