# DiscID_FOUND - system has the DiscID library
# DiscID_INCLUDE_DIRS - the DiscID include directory
# DiscID_LIBRARIES - The libraries needed to use DiscID

find_package(PkgConfig)
pkg_check_modules(PC_DISCID libdiscid)

find_path(DiscID_INCLUDE_DIRS
    NAMES discid/discid.h
    PATHS ${PC_DISCID_INCLUDEDIR})
find_library(DiscID_LIBRARIES NAMES discid ${PC_DISCID_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DiscID FOUND_VAR DiscID_FOUND REQUIRED_VARS DiscID_INCLUDE_DIRS DiscID_LIBRARIES)

mark_as_advanced(DiscID_INCLUDE_DIRS DiscID_LIBRARIES)
