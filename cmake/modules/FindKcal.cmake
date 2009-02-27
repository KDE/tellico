# - Try to find Kcal
# Once done this will define
#
#  KCAL_FOUND - system has Kcal
#  KCAL_INCLUDE_DIR - the Kcal include directory
#  KCAL_LIBRARIES - Link these to use Kcal
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if ( KCAL_INCLUDE_DIR AND KCAL_LIBRARIES )
   # in cache already
   SET(KCAL_FIND_QUIETLY TRUE)
endif ( KCAL_INCLUDE_DIR AND KCAL_LIBRARIES )

FIND_PATH(KCAL_INCLUDE_DIR
    NAMES kcalversion.h
    PATH_SUFFIXES kcal
    PATHS
    ${KDE4_INCLUDE_DIR}
    ${INCLUDE_INSTALL_DIR}
)

FIND_LIBRARY(KCAL_LIBRARIES
    NAMES kcal
    PATHS
    ${KDE4_LIB_DIR}
    ${LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libkcal "libkcal was not found. Need to install from kdepimlibs" KCAL_LIBRARIES KCAL_INCLUDE_DIR)

# show the KCAL_INCLUDE_DIR and KCAL_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KCAL_INCLUDE_DIR KCAL_LIBRARIES )

