# LIBV4L_FOUND - system has the libv4l library
# LIBV4L_INCLUDE_DIR - the libv4l include directory
# LIBV4l_LIBRARIES - The libraries needed to use libv4llibv4l
#
# Redistribution and use is allowed according to the terms of the BSD license.
#
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

FIND_PATH(LIBV4L_INCLUDE_DIR
    NAMES libv4l1.h
)

FIND_LIBRARY(LIBV4L_LIBRARIES
    NAMES v4l1
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libv4l FOUND_VAR LIBV4L_FOUND REQUIRED_VARS LIBV4L_INCLUDE_DIR LIBV4L_LIBRARIES)

mark_as_advanced(LIBV4L_INCLUDE_DIR LIBV4L_LIBRARIES)
