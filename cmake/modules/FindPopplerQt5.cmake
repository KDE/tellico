# - Try to find the Qt5 binding of the Poppler library
# Once done this will define
#
#  POPPLER_QT5_FOUND - system has poppler-qt5
#  POPPLER_QT5_INCLUDE_DIR - the poppler-qt5 include directory
#  POPPLER_QT5_LIBRARIES - Link these to use poppler-qt5
#  POPPLER_QT5_DEFINITIONS - Compiler switches required for using poppler-qt5
#

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls

# Copyright (c) 2006, Wilfried Huss, <wilfried.huss@gmx.at>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.


find_package(PkgConfig)
pkg_check_modules(PC_POPPLERQT5 QUIET poppler-qt5)

set(POPPLER_QT5_DEFINITIONS ${PC_POPPLERQT5_CFLAGS_OTHER})

find_path(POPPLER_QT5_INCLUDE_DIR
  NAMES poppler-qt5.h
  HINTS ${PC_POPPLERQT5_INCLUDEDIR}
  PATH_SUFFIXES poppler/qt5 poppler
)

find_library(POPPLER_QT5_LIBRARY
  NAMES poppler-qt5
  HINTS ${PC_POPPLERQT5_LIBDIR}
)

set(POPPLER_QT5_LIBRARIES ${POPPLER_QT5_LIBRARY})

if (POPPLER_QT5_INCLUDE_DIR AND POPPLER_QT5_LIBRARIES)
  set(POPPLER_QT5_FOUND TRUE)
else ()
  set(POPPLER_QT5_FOUND FALSE)
endif ()
  
if (POPPLER_QT5_FOUND)
  if (NOT PopplerQt5_FIND_QUIETLY)
    message(STATUS "Found poppler-qt5: library: ${POPPLER_QT5_LIBRARIES}, include path: ${POPPLER_QT5_INCLUDE_DIR}")
  endif ()
else ()
  if (PopplerQt5_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find poppler-qt5")
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PopplerQt5 REQUIRED_VARS POPPLER_QT5_INCLUDE_DIR POPPLER_QT5_LIBRARIES)

mark_as_advanced(POPPLER_QT5_INCLUDE_DIR POPPLER_QT5_LIBRARIES)
