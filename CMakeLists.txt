project(Tellico)
set(TELLICO_VERSION "2.3.7+git")

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
option(BUILD_FETCHER_TESTS "Build tests which verify data sources" FALSE)

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

macro_optional_find_package(Nepomuk)
macro_log_feature(NEPOMUK_FOUND "nepomuk" "Support for reading file metadata" "http://www.kde.org" FALSE "4.2.0" "")
if(NEPOMUK_FOUND)
    include_directories(${NEPOMUK_INCLUDE_DIRS})
endif(NEPOMUK_FOUND)

macro_optional_find_package(KdepimLibs)
macro_log_feature(KDEPIMLIBS_FOUND "kdepimlibs" "Support for using the address book and calendar for loans" "http://pim.kde.org" FALSE "4.2.0" "")
if(KDEPIMLIBS_FOUND)
    include_directories(${KDEPIMLIBS_INCLUDE_DIRS})
endif(KDEPIMLIBS_FOUND)

macro_optional_find_package(KdeMultimedia)
macro_log_feature(KDEMULTIMEDIA_FOUND "kdemultimedia" "Support for CDDB searches" "http://multimedia.kde.org" FALSE "4.2.0" "")

# The taglib cmake script is broken for older taglib versions, this works around the issue
set(TAGLIB_INCLUDES)
macro_optional_find_package(Taglib)
macro_log_feature(TAGLIB_FOUND "taglib" "Support for reading multimedia files" "http://www.freshmeat.net/projects/taglib" FALSE "" "")
if(TAGLIB_FOUND)
    add_definitions(${TAGLIB_CFLAGS})
    include_directories(${TAGLIB_INCLUDES})
endif(TAGLIB_FOUND)

macro_optional_find_package(Yaz 2.0)
macro_log_feature(YAZ_FOUND "libyaz" "Support for searching z39.50 databases" "http://www.indexdata.dk/yaz/" FALSE "2.0" "")

macro_optional_find_package(PopplerQt4)
macro_log_feature(POPPLER_QT4_FOUND "libpoppler" "Support for reading PDF files" "http://poppler.freedesktop.org/" FALSE "" "")
if(POPPLER_QT4_FOUND)
    add_definitions(${POPPLER_QT4_DEFINITIONS})
    include_directories(${POPPLER_QT4_INCLUDE_DIR})
endif(POPPLER_QT4_FOUND)

macro_optional_find_package(KSane)
macro_log_feature(KSANE_FOUND "libksane" "Support for adding scanned images to a collection" "http://www.kde.org/" FALSE "4.2.0" "")
if(KSANE_FOUND)
    include_directories(${KSANE_INCLUDE_DIR})
endif(KSANE_FOUND)

macro_optional_find_package(Exempi 2.0)
macro_log_feature(LIBEXEMPI_FOUND "libexempi" "Support for reading PDF/XMP metadata" "http://libopenraw.freedesktop.org/wiki/Exempi" FALSE "2.0" "")
if(LIBEXEMPI_FOUND)
    add_definitions(${LIBEXEMPI_CFLAGS})
    include_directories(${LIBEXEMPI_INCLUDE_DIR})
endif(LIBEXEMPI_FOUND)

macro_optional_find_package(QJSON)
macro_log_feature(QJSON_FOUND "QJSON" "Support for searching OpenLibrary and Freebase" "http://qjson.sourceforge.net" FALSE "" "")
if(QJSON_FOUND)
    include_directories(${QJSON_INCLUDE_DIR})
endif(QJSON_FOUND)

# webcam uses libv4l, which only works on Linux for now
# Linux 2.6.38 removed the videodev.h header
# libv4l 0.8.3 includes a compat header for videodev.h
if(ENABLE_WEBCAM)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
        message("WARNING: Webcam support is not available on your platform")
        set( ENABLE_WEBCAM FALSE )
    else(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
        pkg_check_modules(LIBV4L libv4l1>=0.6)
        macro_log_feature(LIBV4L_FOUND "libv4l" "Support for barcode scanning with a webcam" "http://hansdegoede.livejournal.com/3636.html"  FALSE "" "")
        if(LIBV4L_FOUND)
            if(LIBV4L_VERSION VERSION_LESS "0.8.3" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "2.6.37")
                message("WARNING: libv4l 0.8.3 or later is required for Linux kernel 2.6.38 or later")
                set( ENABLE_WEBCAM FALSE )
            else(LIBV4L_VERSION VERSION_LESS "0.8.3" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "2.6.37")
                include_directories(${LIBV4L_INCLUDE_DIR})
            endif(LIBV4L_VERSION VERSION_LESS "0.8.3" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "2.6.37")
        else(LIBV4L_FOUND)
            set(ENABLE_WEBCAM FALSE)
        endif(LIBV4L_FOUND)
    endif(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
endif(ENABLE_WEBCAM)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -Wextra -fno-check-new -Woverloaded-virtual")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_definitions(${QT_DEFINITIONS}
                ${KDE4_DEFINITIONS}
                ${YAZ_CFLAGS})
add_definitions(-DQT_STL -DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_STL -DQT3_SUPPORT)

include_directories(${KDE4_INCLUDES}
                    ${QT_INCLUDES}
                    ${QIMAGEBLITZ_INCLUDES}
                    ${LIBXML2_INCLUDE_DIR}
                    ${LIBXSLT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${Tellico_SOURCE_DIR}/src/core
                    ${Tellico_SOURCE_DIR}/src/3rdparty)

if(KDE_VERSION VERSION_GREATER 4.4)
    set(ENABLE_KNEWSTUFF3 TRUE)
else(KDE_VERSION VERSION_GREATER 4.4)
    set(ENABLE_KNEWSTUFF3 FALSE)
endif(KDE_VERSION VERSION_GREATER 4.4)

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
add_subdirectory(doc)

if(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD|Linux|kFreeBSD|NetBSD|OpenBSD|SunOS|Darwin")
    message("WARNING: CDDB Lookup is not supported on your platform")
endif(NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD|Linux|kFreeBSD|NetBSD|OpenBSD|SunOS|Darwin")

########## Wrap tests results around the tests done within the source

macro_bool_to_01(TAGLIB_FOUND HAVE_TAGLIB)
macro_bool_to_01(POPPLER_QT4_FOUND HAVE_POPPLER)
macro_bool_to_01(LIBEXEMPI_FOUND HAVE_EXEMPI)
macro_bool_to_01(YAZ_FOUND HAVE_YAZ)
macro_bool_to_01(KSANE_FOUND HAVE_KSANE)
macro_bool_to_01(KDEMULTIMEDIA_FOUND HAVE_KCDDB)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KABC)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KCAL)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KXMLRPC)
macro_bool_to_01(LIBV4L_FOUND HAVE_V4L)
macro_bool_to_01(QJSON_FOUND HAVE_QJSON)
macro_bool_to_01(NEPOMUK_FOUND HAVE_NEPOMUK)
macro_bool_to_01(ENABLE_KNEWSTUFF3 ENABLE_KNEWSTUFF3)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

########### install files ###############

install(FILES tellico.dtd tellico.tips DESTINATION ${TELLICO_DATA_INSTALL_DIR})
install(PROGRAMS tellico.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES tellico.xml DESTINATION  ${XDG_MIME_INSTALL_DIR})
install(FILES x-tellico.desktop DESTINATION ${MIME_INSTALL_DIR}/application)

if(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    macro_display_feature_log()
endif(CMAKE_SOURCE_DIR STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")

