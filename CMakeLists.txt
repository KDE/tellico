# FindKF5 requires CMake >= 2.8.12
cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(Tellico)
set(TELLICO_VERSION "3.1.3+git")

set(QT_MIN_VERSION "5.4.0")

# FindPoppler was added to ECM in 5.19
find_package(ECM 5.19 REQUIRED NO_MODULE)

CMAKE_POLICY(SET CMP0028 OLD)
if(POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif(POLICY CMP0063)

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
include(ECMUninstallTarget)
include(FeatureSummary)

include(KDEInstallDirs)
#include(KDECompilerSettings)
include(KDEFrameworkCompilerSettings)
include(KDECMakeSettings)

############# Options #################

option(ENABLE_AMAZON       "Enable Amazon.com searching" TRUE)
option(ENABLE_IMDB         "Enable IMDb searching" TRUE)
option(ENABLE_CDTEXT       "Enable cdtext" TRUE)
option(ENABLE_WEBCAM       "Enable support for webcams" FALSE)
option(BUILD_TESTS         "Build the tests" TRUE)
option(BUILD_FETCHER_TESTS "Build tests which verify data sources" FALSE)

include(CheckSymbolExists)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strupr "string.h" HAVE_STRUPR)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Widgets
    Xml
    DBus
    Test
    Network
)

find_package(KF5 REQUIRED COMPONENTS
    Archive
    Codecs
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DocTools
    GuiAddons
    IconThemes
    ItemModels
    I18n
    JobWidgets
    KIO
    Solid
    Wallet
    WidgetsAddons
    WindowSystem
    XmlGui
)
find_package(KF5KHtml REQUIRED NO_MODULE)

include(MacroBoolTo01)

find_package(Gettext REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

find_package(KF5FileMetaData)
set_package_properties(KF5FileMetaData PROPERTIES
    DESCRIPTION "Support for reading file metadata"
    URL "http://www.kde.org"
    TYPE OPTIONAL)

find_package(KF5NewStuff)
set_package_properties(KF5NewStuff PROPERTIES
    DESCRIPTION "Support for fetching new templates and scripts"
    URL "http://www.kde.org"
    TYPE OPTIONAL)

find_package(KF5Sane)
set_package_properties(KF5Sane PROPERTIES
    DESCRIPTION "Support for adding scanned images to a collection"
    URL "http://www.kde.org"
    TYPE OPTIONAL)
if(KF5Sane_FOUND)
    include_directories(${KF5Sane_INCLUDE_DIR})
endif(KF5Sane_FOUND)

# Use pkg-config just to check QImageBlitz library version since CMake doesn't
# can't just use pkg-config because qimageblitz.pc doesn't seem to have correct includes and linking
pkg_check_modules (QImageBlitz qimageblitz>=5.0)
if(QImageBlitz_FOUND)
    find_package(QImageBlitz 5.0)
    set_package_properties(QImageBlitz PROPERTIES
        DESCRIPTION "Support for image gradients in the default template"
        URL "http://api.kde.org/kdesupport-api/kdesupport-apidocs/qimageblitz/html/"
        TYPE OPTIONAL)
        include_directories(${QIMAGEBLITZ_INCLUDES})
endif(QImageBlitz_FOUND)

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

# There is a port of libkcddb to use KF5 style linking and headers
find_package(KF5Cddb)
set_package_properties(KF5Cddb PROPERTIES
    DESCRIPTION "Support for CDDB searches"
    URL "https://cgit.kde.org/libkcddb.git"
    TYPE OPTIONAL)

find_package(Taglib)
set_package_properties(Taglib PROPERTIES
    DESCRIPTION "Support for reading multimedia files"
    URL "http://taglib.github.io"
    TYPE OPTIONAL)
if(TAGLIB_FOUND)
    add_definitions(${TAGLIB_CFLAGS})
    include_directories(${TAGLIB_INCLUDES})
endif(TAGLIB_FOUND)

find_package(Yaz 2.0)
set_package_properties(Yaz PROPERTIES
    DESCRIPTION "Support for searching z39.50 databases"
    URL "http://www.indexdata.dk/yaz/"
    TYPE OPTIONAL)
if(Yaz_FOUND)
    include_directories(${Yaz_INCLUDE_DIRS})
endif(Yaz_FOUND)

# FindPoppler is part of ECM >= 5.19
find_package(Poppler COMPONENTS Qt5)

find_package(Exempi 2.0)
set_package_properties(Exempi PROPERTIES
    DESCRIPTION "Support for reading PDF/XMP metadata"
    URL "http://libopenraw.freedesktop.org/wiki/Exempi/"
    TYPE OPTIONAL)
if(Exempi_FOUND)
    include_directories(${Exempi_INCLUDE_DIRS})
endif(Exempi_FOUND)

find_package(Btparse)
set_package_properties(Btparse PROPERTIES
    DESCRIPTION "External support for parsing and processing BibTeX data files"
    URL "https://metacpan.org/release/Text-BibTeX"
    TYPE OPTIONAL)
if(Btparse_FOUND)
    include_directories(${Btparse_INCLUDE_DIRS})
    set(TELLICO_BTPARSE_LIBS ${Btparse_LIBRARIES})
else(Btparse_FOUND)
    set(TELLICO_BTPARSE_LIBS btparse-tellico)
endif(Btparse_FOUND)

find_package(CDIO)
set_package_properties(CDIO PROPERTIES
    DESCRIPTION "Support for reading cdtext from audio CDs"
    URL "https://www.gnu.org/software/libcdio/"
    TYPE OPTIONAL)
if(CDIO_FOUND)
    include_directories(${CDIO_INCLUDE_DIRS})
endif(CDIO_FOUND)

find_package(Csv 3.0)
set_package_properties(Csv PROPERTIES
    DESCRIPTION "External support for reading CSV files"
    URL "http://sourceforge.net/projects/libcsv/"
    TYPE OPTIONAL)
if(Csv_FOUND)
    include_directories(${Csv_INCLUDE_DIRS})
    set(TELLICO_CSV_LIBS ${Csv_LIBRARIES})
else(Csv_FOUND)
    set(TELLICO_CSV_LIBS csv-tellico)
endif(Csv_FOUND)

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
add_definitions(-DQT_STL -DQT_NO_CAST_FROM_ASCII -DQT_NO_URL_CAST_FROM_STRING)
remove_definitions(-DQT_NO_STL)

include_directories(${LIBXML2_INCLUDE_DIR}
                    ${LIBXSLT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${Tellico_SOURCE_DIR}/src/config
                    ${Tellico_SOURCE_DIR}/src/3rdparty)

set(TELLICO_DATA_INSTALL_DIR ${KDE_INSTALL_DATADIR}/tellico)

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(xslt)
add_subdirectory(doc)

########## Wrap tests results around the tests done within the source

macro_bool_to_01(QIMAGEBLITZ_FOUND HAVE_QIMAGEBLITZ)
macro_bool_to_01(TAGLIB_FOUND HAVE_TAGLIB)
macro_bool_to_01(Poppler_Qt5_FOUND HAVE_POPPLER)
macro_bool_to_01(Exempi_FOUND HAVE_EXEMPI)
macro_bool_to_01(Yaz_FOUND HAVE_YAZ)
macro_bool_to_01(KF5Sane_FOUND HAVE_KSANE)
macro_bool_to_01(Libkcddb_FOUND HAVE_KCDDB)
macro_bool_to_01(KF5Cddb_FOUND HAVE_KF5KCDDB)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KABC)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KCAL)
macro_bool_to_01(LIBV4L_FOUND HAVE_V4L)
macro_bool_to_01(KF5NewStuff_FOUND ENABLE_KNEWSTUFF3)
macro_bool_to_01(Btparse_FOUND HAVE_LIBBTPARSE)
macro_bool_to_01(CDIO_FOUND HAVE_CDIO)
macro_bool_to_01(Csv_FOUND HAVE_LIBCSV)
macro_bool_to_01(KF5FileMetaData_FOUND HAVE_KFILEMETADATA)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

########### install files ###############

install(PROGRAMS org.kde.tellico.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES tellico.dtd tellico.tips DESTINATION ${TELLICO_DATA_INSTALL_DIR})
install(FILES tellico.xml DESTINATION  ${KDE_INSTALL_MIMEDIR})
install(FILES org.kde.tellico.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
