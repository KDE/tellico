# Btparse_FOUND - system has the btparse library
# Btparse_INCLUDE_DIRS - the btparse include directory
# Btparse_LIBRARIES - The libraries needed to use btparse

find_path(Btparse_INCLUDE_DIRS
    NAMES btparse.h
)
find_library(Btparse_LIBRARIES NAMES btparse)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Btparse FOUND_VAR Btparse_FOUND REQUIRED_VARS Btparse_INCLUDE_DIRS Btparse_LIBRARIES)

mark_as_advanced(Btparse_INCLUDE_DIRS Btparse_LIBRARIES)
