# LIBV4L_FOUND - system has the libv4l library
# LIBV4L_INCLUDE_DIR - the libv4l include directory
# LIBV4l_LIBRARIES - The libraries needed to use libv4llibv4l
#
# Redistribution and use is allowed according to the terms of the BSD license.
#
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

#in cache already
if(LIBV4L_INCLUDE_DIR AND LIBV4L_LIBRARIES)
  set(LIBV4L_FIND_QUIETLY TRUE)
endif(LIBV4L_INCLUDE_DIR AND LIBV4L_LIBRARIES)

FIND_PATH(LIBV4L_INCLUDE_DIR
    NAMES libv4l1.h
    PATHS
    ${KDE4_INCLUDE_DIR}
    ${INCLUDE_INSTALL_DIR}
)

FIND_LIBRARY(LIBV4L_LIBRARIES
    NAMES v4l1
    PATHS
    ${KDE4_LIB_DIR}
    ${LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libv4l DEFAULT_MSG LIBV4L_INCLUDE_DIR LIBV4L_LIBRARIES)

mark_as_advanced(LIBV4L_INCLUDE_DIR LIBV4L_LIBRARIES)
