# YAZ_FOUND - system has the YAZ library
# YAZ_INCLUDE_DIR - the YAZ include directory
# YAZ_LIBRARIES - The libraries needed to use YAZ

#in cache already
if(YAZ_INCLUDE_DIR AND YAZ_LIBRARIES)
  set(YAZ_FIND_QUIETLY TRUE)
endif(YAZ_INCLUDE_DIR AND YAZ_LIBRARIES)

include(FindPkgConfig)
pkg_check_modules(YAZ yaz)

find_path(YAZ_INCLUDE_DIR
    NAMES yaz-version.h
    PATH_SUFFIXES yaz
    PATHS ${YAZ_INCLUDEDIR})

find_library(YAZ_LIBRARIES NAMES yaz ${YAZ_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libyaz DEFAULT_MSG YAZ_INCLUDE_DIR YAZ_LIBRARIES)

mark_as_advanced(YAZ_INCLUDE_DIR YAZ_LIBRARIES)
