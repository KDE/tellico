# BTPARSE_FOUND - system has the btparse library
# BTPARSE_INCLUDE_DIR - the btparse include directory
# BTPARSE_LIBRARIES - The libraries needed to use btparse

#in cache already
if(BTPARSE_INCLUDE_DIR AND BTPARSE_LIBRARIES)
  set(BTPARSE_FIND_QUIETLY TRUE)
endif(BTPARSE_INCLUDE_DIR AND BTPARSE_LIBRARIES)

find_path(BTPARSE_INCLUDE_DIR
    NAMES btparse.h
)
find_library(BTPARSE_LIBRARIES NAMES btparse)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(btparse DEFAULT_MSG BTPARSE_INCLUDE_DIR BTPARSE_LIBRARIES)

mark_as_advanced(BTPARSE_INCLUDE_DIR BTPARSE_LIBRARIES)
