project(Tellico)
set(TELLICO_VERSION "2.1svn")

cmake_minimum_required(VERSION 2.6.2)

# http://www.cmake.org/Wiki/CMake_Useful_Variables
# automatically add CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR
# to the include directories in every processed CMakeLists.txt
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

############# Options #################

option(ENABLE_AMAZON "Enable Amazon.com searching" TRUE)
option(ENABLE_IMDB "Enable IMDb searching" TRUE)
option(ENABLE_CDTEXT "Enable cdtext" TRUE)
option(ENABLE_WEBCAM "Enable support for webcams" FALSE)

include(CheckTypeSize)
check_type_size("unsigned char" SIZEOF_UNSIGNED_CHAR)
check_type_size("unsigned short" SIZEOF_UNSIGNED_SHORT)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size("unsigned int" SIZEOF_UNSIGNED_INT)
check_type_size("short" SIZEOF_SHORT)
check_type_size("long" SIZEOF_LONG)
check_type_size("int" SIZEOF_INT)

include(CheckSymbolExists)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strupr "string.h" HAVE_STRUPR)

find_package(KDE4 4.2.0 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroBoolTo01)
include(MacroLogFeature)

find_package(Gettext REQUIRED)
find_package(QImageBlitz REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

# The taglib cmake script is broken, this works around the issue
set(TAGLIB_INCLUDES)

macro_optional_find_package(Kabc)
macro_log_feature(KABC_FOUND "kabc" "Support for reading address book" "http://pim.kde.org" FALSE "4.2.0" "")

macro_optional_find_package(Kcal)
macro_log_feature(KCAL_FOUND "kcal" "Support for adding loans to calendar" "http://pim.kde.org" FALSE "4.2.0" "")
# for now, kcal doesn't compile
set(KCAL_FOUND FALSE)

macro_optional_find_package(Kcddb)
macro_log_feature(KCDDB_FOUND "libkcddb" "Support for CDDB searches" "http://multimedia.kde.org" FALSE "4.2.0" "")

macro_optional_find_package(Taglib)
macro_log_feature(TAGLIB_FOUND "taglib" "Support for reading multimedia files" "http://www.freshmeat.net/projects/taglib" FALSE "" "")

macro_optional_find_package(Yaz 2.0)
macro_log_feature(YAZ_FOUND "libyaz" "Support for searching z39.50 databases" "http://www.indexdata.dk/yaz/" FALSE "2.0" "")

macro_optional_find_package(PopplerQt4)
macro_log_feature(POPPLER_QT4_FOUND "libpoppler" "Support for reading PDF files" "http://poppler.freedesktop.org/" FALSE "" "")
if(POPPLER_QT4_FOUND)
    add_definitions(${POPPLER_QT4_DEFINITIONS})
    include_directories(${POPPLER_QT4_INCLUDE_DIR})
endif(POPPLER_QT4_FOUND)

macro_optional_find_package(KSane)
macro_log_feature(KSANE_FOUND "libksane" "Support for scanning images" "http://www.kde.org/" FALSE "4.2.0" "")
if(KSANE_FOUND)
    include_directories(${KSANE_INCLUDE_DIR})
endif(KSANE_FOUND)

macro_optional_find_package(Exempi 2.0)
macro_log_feature(LIBEXEMPI_FOUND "libexempi" "Support for reading PDF/XMP metadata" "http://libopenraw.freedesktop.org/wiki/Exempi" FALSE "2.0" "")
if(LIBEXEMPI_FOUND)
    add_definitions(${LIBEXEMPI_CFLAGS})
    include_directories(${LIBEXEMPI_INCLUDE_DIR})
endif(LIBEXEMPI_FOUND)

macro_optional_find_package(V4l)
macro_log_feature(LIBV4L_FOUND "libv4l" "Support for barcode scanning" "http://hansdegoede.livejournal.com/3636.html" FALSE "" "Required for using a webcam")
if(LIBV4L_FOUND)
    include_directories(${LIBV4L_INCLUDE_DIR})
else(LIBV4L_FOUND)
    set( ENABLE_WEBCAM FALSE )
endif(LIBV4L_FOUND)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -pedantic -Wextra -fno-check-new")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_definitions(${QT_DEFINITIONS}
                ${KDE4_DEFINITIONS}
                ${TAGLIB_CFLAGS}
                ${YAZ_CFLAGS})
add_definitions(-DQT_STL -DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_STL -DQT3_SUPPORT)

include_directories(${KDE4_INCLUDES}
                    ${QT_INCLUDES}
                    ${LIBXML2_INCLUDE_DIR}
                    ${LIBXSLT_INCLUDE_DIR}
                    ${TAGLIB_INCLUDES}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${Tellico_SOURCE_DIR}/src/core
                    ${Tellico_SOURCE_DIR}/src/3rdparty)

# is there a better way to disable qt3support library?
get_directory_property(include_dirs INCLUDE_DIRECTORIES)
if(QT_QT3SUPPORT_FOUND)
  string(REPLACE ${QT_QT3SUPPORT_INCLUDE_DIR} "" include_dirs "${include_dirs}")
endif(QT_QT3SUPPORT_FOUND)
set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs}")

set(TELLICO_DATA_INSTALL_DIR ${DATA_INSTALL_DIR}/tellico)

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(xslt)

if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD|Linux|kFreeBSD|NetBSD|OpenBSD|SunOS|Darwin")
    message("WARNING: CDDB Lookup is not supported on your platform")
endif(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD|Linux|kFreeBSD|NetBSD|OpenBSD|SunOS|Darwin")

if(ENABLE_WEBCAM AND NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    message("WARNING: Webcam support is not available on your platform")
    set( ENABLE_WEBCAM FALSE )
endif(ENABLE_WEBCAM AND NOT CMAKE_SYSTEM_NAME MATCHES "Linux")

########## Wrap tests results around the tests done within the source

macro_bool_to_01(TAGLIB_FOUND HAVE_TAGLIB)
macro_bool_to_01(POPPLER_QT4_FOUND HAVE_POPPLER)
macro_bool_to_01(LIBEXEMPI_FOUND HAVE_EXEMPI)
macro_bool_to_01(YAZ_FOUND HAVE_YAZ)
macro_bool_to_01(KSANE_FOUND HAVE_KSANE)
macro_bool_to_01(KCDDB_FOUND HAVE_KCDDB)
macro_bool_to_01(KABC_FOUND HAVE_KABC)
macro_bool_to_01(KCAL_FOUND HAVE_KCAL)
macro_bool_to_01(LIBV4L_FOUND HAVE_V4L)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

########### install files ###############

install(FILES tellico.dtd tellico.tips DESTINATION ${TELLICO_DATA_INSTALL_DIR})
install(FILES tellico.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES tellico.xml DESTINATION  ${XDG_MIME_INSTALL_DIR})
install(FILES x-tellico.desktop DESTINATION ${MIME_INSTALL_DIR}/application)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
