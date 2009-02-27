# - Try to find Kabc
# Once done this will define
#
#  KABC_FOUND - system has Kcal
#  KABC_INCLUDE_DIR - the Kcal include directory
#  KABC_LIBRARIES - Link these to use Kcal
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if ( KABC_INCLUDE_DIR AND KABC_LIBRARIES )
   # in cache already
   SET(KABC_FIND_QUIETLY TRUE)
endif ( KABC_INCLUDE_DIR AND KABC_LIBRARIES )

FIND_PATH(KABC_INCLUDE_DIR
    NAMES addressbook.h
    PATH_SUFFIXES kabc
    PATHS
    ${KDE4_INCLUDE_DIR}
    ${INCLUDE_INSTALL_DIR}
)

FIND_LIBRARY(KABC_LIBRARIES
    NAMES kabc
    PATHS
    ${KDE4_LIB_DIR}
    ${LIB_INSTALL_DIR}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(kabc "libkabc was not found. Need to install from kdepimlibs" KABC_LIBRARIES KABC_INCLUDE_DIR)

# show the KABC_INCLUDE_DIR and KABC_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(KABC_INCLUDE_DIR KABC_LIBRARIES )

