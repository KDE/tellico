project(Tellico)
set(TELLICO_VERSION "2.9.90+git")

# FindKF5 requires CMake >= 2.8.12
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.3.0")

find_package(ECM 1.3.0 REQUIRED NO_MODULE)

# http://www.cmake.org/Wiki/CMake_Useful_Variables
# automatically add CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR
# to the include directories in every processed CMakeLists.txt
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${ECM_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

#include(ECMAddTests)
#include(ECMMarkNonGuiExecutable)
#include(ECMInstallIcons)
include(FeatureSummary)

include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)

############# Options #################

option(ENABLE_AMAZON       "Enable Amazon.com searching" TRUE)
option(ENABLE_IMDB         "Enable IMDb searching" TRUE)
option(ENABLE_CDTEXT       "Enable cdtext" TRUE)
option(ENABLE_WEBCAM       "Enable support for webcams" FALSE)
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

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
    Test
)

find_package(KF5 REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    IconThemes
    I18n
    KIO
    WidgetsAddons
    XmlGui
    JobWidgets
    KDELibs4Support
)
find_package(KF5KHtml NO_MODULE)

#find_package(KF5 REQUIRED Config ConfigWidgets CoreAddons IconThemes I18n KIO
#             Notifications WidgetsAddons WindowSystem Parts Solid DocTools XmlGui JobWidgets)

#find_package(KDE4 4.2.0 REQUIRED)
#include(KDE4Defaults)
include(MacroLibrary)
include(MacroBoolTo01)
#include(MacroLogFeature)

find_package(Gettext REQUIRED)
find_package(QImageBlitz REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

#macro_optional_find_package(Nepomuk)
#macro_log_feature(NEPOMUK_FOUND "nepomuk" "Support for reading file metadata" "http://www.kde.org" FALSE "4.2.0" "")
#find_package(Nepomuk)
#set_package_properties(Nepomuk PROPERTIES
#    DESCRIPTION "Support for reading file metadata"
#    URL "http://www.kde.org"
#    TYPE OPTIONAL)
#if(NEPOMUK_FOUND)
#    include_directories(${NEPOMUK_INCLUDE_DIRS})
#endif(NEPOMUK_FOUND)

#macro_optional_find_package(KdepimLibs 4.5)
#macro_log_feature(KDEPIMLIBS_FOUND "kdepimlibs" "Support for using the address book and calendar for loans" "http://pim.kde.org" FALSE "4.5.0" "")
#find_package(KdepimLibs 4.5)
#set_package_properties(KdepimLibs PROPERTIES
#    DESCRIPTION "Support for using the address book and calendar for loans"
#    URL "http://pim.kde.org"
#    TYPE OPTIONAL)
if(KDEPIMLIBS_FOUND)
    include_directories(${KDEPIMLIBS_INCLUDE_DIRS})
endif(KDEPIMLIBS_FOUND)

#macro_optional_find_package(KdeMultimedia)
#macro_log_feature(KDEMULTIMEDIA_FOUND "kdemultimedia" "Support for CDDB searches" "http://multimedia.kde.org" FALSE "4.2.0" "")
#find_package(KdeMultimedia 4.2)
#set_package_properties(KdeMultimedia PROPERTIES
#    DESCRIPTION "Support for CDDB searches"
#    URL "http://multimedia.kde.org"
#    TYPE OPTIONAL)

#macro_optional_find_package(Taglib)
#macro_log_feature(TAGLIB_FOUND "taglib" "Support for reading multimedia files" "http://www.freshmeat.net/projects/taglib" FALSE "" "")
find_package(Taglib)
set_package_properties(Taglib PROPERTIES
    DESCRIPTION "Support for reading multimedia files"
    URL "http://taglib.github.io"
    TYPE OPTIONAL)
if(TAGLIB_FOUND)
    add_definitions(${TAGLIB_CFLAGS})
    include_directories(${TAGLIB_INCLUDES})
endif(TAGLIB_FOUND)

#macro_optional_find_package(Yaz 2.0)
#macro_log_feature(YAZ_FOUND "libyaz" "Support for searching z39.50 databases" "http://www.indexdata.dk/yaz/" FALSE "2.0" "")
find_package(Yaz 2.0)
set_package_properties(Yaz PROPERTIES
    DESCRIPTION "Support for searching z39.50 databases"
    URL "http://www.indexdata.dk/yaz/"
    TYPE OPTIONAL)

#macro_optional_find_package(PopplerQt4)
#macro_log_feature(POPPLER_QT4_FOUND "libpoppler" "Support for reading PDF files" "http://poppler.freedesktop.org/" FALSE "" "")
#find_package(PopplerQt4)
#set_package_properties(PopplerQt4 PROPERTIES
#    DESCRIPTION "Support for reading PDF files"
#    URL "http://poppler.freedesktop.org"
#    TYPE OPTIONAL)
if(POPPLER_QT4_FOUND)
    add_definitions(${POPPLER_QT4_DEFINITIONS})
    include_directories(${POPPLER_QT4_INCLUDE_DIR})
endif(POPPLER_QT4_FOUND)

#macro_optional_find_package(KSane)
#macro_log_feature(KSANE_FOUND "libksane" "Support for adding scanned images to a collection" "http://www.kde.org/" FALSE "4.2.0" "")
#find_package(KSane)
#set_package_properties(KSane PROPERTIES
#    DESCRIPTION "Support for adding scanned images to a collection"
#    URL "http://www.kde.org"
#    TYPE OPTIONAL)
if(KSANE_FOUND)
    include_directories(${KSANE_INCLUDE_DIR})
endif(KSANE_FOUND)

#macro_optional_find_package(Exempi 2.0)
#macro_log_feature(LIBEXEMPI_FOUND "libexempi" "Support for reading PDF/XMP metadata" "http://libopenraw.freedesktop.org/wiki/Exempi" FALSE "2.0" "")
find_package(Exempi 2.0)
set_package_properties(Exempi PROPERTIES
    DESCRIPTION "Support for reading PDF/XMP metadata"
    URL "http://libopenraw.freedesktop.org/wiki/Exempi/"
    TYPE OPTIONAL)
if(LIBEXEMPI_FOUND)
    add_definitions(${LIBEXEMPI_CFLAGS})
    include_directories(${LIBEXEMPI_INCLUDE_DIR})
endif(LIBEXEMPI_FOUND)

#macro_optional_find_package(QJSON)
#macro_log_feature(QJSON_FOUND "QJSON" "Support for searching additional data sources, such as OpenLibrary and Freebase" "http://qjson.sourceforge.net" FALSE "" "")
#find_package(QJSON)
#set_package_properties(QJSON PROPERTIES
#    DESCRIPTION "Support for searching additional data sources, such as OpenLibrary and Freebase"
#    URL "http://qjson.sourceforge.net"
#    TYPE OPTIONAL)
if(QJSON_FOUND)
    include_directories(${QJSON_INCLUDE_DIR})
endif(QJSON_FOUND)

#macro_optional_find_package(Btparse)
#macro_log_feature(BTPARSE_FOUND "btparse" "External support for parsing and processing BibTeX data files" "https://metacpan.org/release/Text-BibTeX" FALSE "" "")
find_package(Btparse)
set_package_properties(Btparse PROPERTIES
    DESCRIPTION "External support for parsing and processing BibTeX data files"
    URL "https://metacpan.org/release/Text-BibTeX"
    TYPE OPTIONAL)
if(BTPARSE_FOUND)
    include_directories(${BTPARSE_INCLUDE_DIR})
    set(TELLICO_BTPARSE_LIBS ${BTPARSE_LIBRARIES})
else(BTPARSE_FOUND)
    set(TELLICO_BTPARSE_LIBS btparse-tellico)
endif(BTPARSE_FOUND)

#macro_optional_find_package(DiscID)
#macro_log_feature(DISCID_FOUND "libdiscid" "Support for reading disc IDs from audio CDs" "http://musicbrainz.org/doc/libdiscid" FALSE "" "")
find_package(DiscID)
set_package_properties(DiscID PROPERTIES
    DESCRIPTION "Support for reading disc IDs from audio CDs"
    URL "http://musicbrainz.org/doc/libdiscid"
    TYPE OPTIONAL)
if(DISCID_FOUND)
    include_directories(${DISCID_INCLUDE_DIR})
endif(DISCID_FOUND)

#macro_optional_find_package(Csv 3.0)
#macro_log_feature(CSV_FOUND "libcsv" "External support for reading CSV files" "http://sourceforge.net/projects/libcsv/" FALSE "3.0" "")
find_package(Csv 3.0)
set_package_properties(Csv PROPERTIES
    DESCRIPTION "External support for reading CSV files"
    URL "http://sourceforge.net/projects/libcsv/"
    TYPE OPTIONAL)
if(CSV_FOUND)
    include_directories(${CSV_INCLUDE_DIR})
    set(TELLICO_CSV_LIBS ${CSV_LIBRARIES})
else(CSV_FOUND)
    set(TELLICO_CSV_LIBS csv-tellico)
endif(CSV_FOUND)

# webcam uses libv4l, which only works on Linux for now
# Linux 2.6.38 removed the videodev.h header
# libv4l 0.8.3 includes a compat header for videodev.h
if(ENABLE_WEBCAM)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
        message("WARNING: Webcam support is not available on your platform")
        set( ENABLE_WEBCAM FALSE )
    else(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
        pkg_check_modules(LIBV4L libv4l1>=0.6)
        #macro_log_feature(LIBV4L_FOUND "libv4l" "Support for barcode scanning with a webcam" "http://hansdegoede.livejournal.com/3636.html"  FALSE "" "")
        set_package_properties(LIBV4L PROPERTIES
            DESCRIPTION "Support for barcode scanning with a webcam"
            URL "http://hansdegoede.livejournal.com/3636.html"
            TYPE OPTIONAL)
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
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -Wextra -fno-check-new -Woverloaded-virtual")
    # remove -Wno-deprecated when porting to KF5 is more advanced
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -fno-check-new -Woverloaded-virtual -Wno-deprecated -Wno-deprecated-declarations")
endif(CMAKE_COMPILER_IS_GNUCXX)

#add_definitions(${QT_DEFINITIONS}
#                ${KDE4_DEFINITIONS}
#                ${YAZ_CFLAGS})
add_definitions(-DQT_STL -DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_STL -DQT3_SUPPORT)

include_directories(${QIMAGEBLITZ_INCLUDES}
                    ${LIBXML2_INCLUDE_DIR}
                    ${LIBXSLT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${Tellico_SOURCE_DIR}/src/core
                    ${Tellico_SOURCE_DIR}/src/3rdparty)

# TODO
set(ENABLE_KNEWSTUFF3 FALSE)
set(ENABLE_KNEWSTUFF2 FALSE)

set(TELLICO_DATA_INSTALL_DIR ${KDE_INSTALL_DATADIR}/tellico)

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
macro_bool_to_01(LIBV4L_FOUND HAVE_V4L)
macro_bool_to_01(QJSON_FOUND HAVE_QJSON)
macro_bool_to_01(NEPOMUK_FOUND HAVE_NEPOMUK)
macro_bool_to_01(ENABLE_KNEWSTUFF3 ENABLE_KNEWSTUFF3)
macro_bool_to_01(BTPARSE_FOUND HAVE_LIBBTPARSE)
macro_bool_to_01(DISCID_FOUND HAVE_DISCID)
macro_bool_to_01(CSV_FOUND HAVE_LIBCSV)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

########### install files ###############

install(FILES tellico.dtd tellico.tips DESTINATION ${TELLICO_DATA_INSTALL_DIR})
install(PROGRAMS tellico.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES tellico.xml DESTINATION  ${KDE_INSTALL_MIMEDIR})
install(FILES tellico.appdata.xml DESTINATION ${KDE_INSTALL_DATAROOTDIR}/appdata )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
