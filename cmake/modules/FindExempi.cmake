# LIBEXEMPI_FOUND - system has the LIBEXEMPI library
# LIBEXEMPI_INCLUDE_DIR - the LIBEXEMPI include directory
# LIBEXEMPI_LIBRARIES - The libraries needed to use LIBEXEMPI

include(UsePkgConfig)
pkgconfig(exempi-2.0 _LIBEXEMPIINCDIR _LIBEXEMPILINKDIR _LIBEXEMPILINKFLAGS _LIBEXEMPICFLAGS)
set(LIBEXEMPI_DEFINITIONS ${_LIBEXEMPICFLAGS})
   
if(LIBEXEMPI_INCLUDE_DIR AND LIBEXEMPI_LIBRARIES)
  set(LIBEXEMPI_FIND_QUIETLY TRUE)
endif(LIBEXEMPI_INCLUDE_DIR AND LIBEXEMPI_LIBRARIES)

find_path(LIBEXEMPI_INCLUDE_DIR
    NAMES exempi/xmp.h
    PATH_SUFFIXES exempi-2.0
    PATHS ${_LIBEXEMPIINCDIR})
find_library(LIBEXEMPI_LIBRARIES NAMES exempi ${_LIBEXEMPILINKDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libexempi DEFAULT_MSG LIBEXEMPI_INCLUDE_DIR LIBEXEMPI_LIBRARIES)

mark_as_advanced(LIBEXEMPI_INCLUDE_DIR LIBEXEMPI_LIBRARIES)
