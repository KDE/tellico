# DISCID_FOUND - system has the DiscID library
# DISCID_INCLUDE_DIR - the DiscID include directory
# DISCID_LIBRARIES - The libraries needed to use DiscID

#in cache already
if(DISCID_INCLUDE_DIR AND DISCID_LIBRARIES)
  set(DISCID_FIND_QUIETLY TRUE)
endif(DISCID_INCLUDE_DIR AND DISCID_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_DISCID libdiscid)

find_path(DISCID_INCLUDE_DIR
    NAMES discid/discid.h
    PATHS ${PC_DISCID_INCLUDEDIR})
find_library(DISCID_LIBRARIES NAMES discid ${PC_DISCID_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DiscID DEFAULT_MSG DISCID_INCLUDE_DIR DISCID_LIBRARIES)

mark_as_advanced(DISCID_INCLUDE_DIR DISCID_LIBRARIES)
