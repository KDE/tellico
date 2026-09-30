# FindPoppler requires CMake >= 3.16
cmake_minimum_required(VERSION 3.16)

project(Tellico)
set(TELLICO_VERSION "4.3+git")
set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")
set(CMAKE_CXX_STANDARD 20)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/Useful-Variables
# automatically add CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR
# to the include directories in every processed CMakeLists.txt
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
                      ${ECM_MODULE_PATH})

include(ECMAddAppIcon)
include(ECMUninstallTarget)
include(FeatureSummary)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

############# Options #################

option(BUILD_DOC           "Whether to build the documentation" TRUE)
option(ENABLE_CDTEXT       "Enable cdtext" TRUE)
option(ENABLE_WEBCAM       "Enable support for webcams" FALSE)
option(BUILD_FETCHER_TESTS "Build tests which verify data sources" FALSE)
option(ENABLE_BTPARSE      "Build with bibtex importing using btparse, whether internal or external" TRUE)

# btparse does not compile with msvc
if(ENABLE_BTPARSE AND MSVC)
    set(ENABLE_BTPARSE FALSE)
endif()

include(CheckSymbolExists)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strupr "string.h" HAVE_STRUPR)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Concurrent
    Core
    DBus
    Network
    PrintSupport
    Test
    WebEngineWidgets
    Widgets
    Xml
)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG OPTIONAL_COMPONENTS
    Charts
)

find_package(KF6 REQUIRED COMPONENTS
    Archive
    Codecs
    Completion
    Config
    ConfigWidgets
    CoreAddons
    Crash
    GuiAddons
    IconThemes
    ItemModels
    I18n
    JobWidgets
    KIO
    Solid
    Sonnet
    TextWidgets
    WidgetsAddons
    XmlGui
    ColorScheme
)

include(MacroBoolTo01)

find_package(Gettext REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

if(BUILD_DOC)
    find_package(KF6DocTools REQUIRED)
    set_package_properties(KF6DocTools PROPERTIES
        DESCRIPTION "Tools to generate documentation"
        TYPE REQUIRED)
endif()

find_package(KF6FileMetaData)
set_package_properties(KF6FileMetaData PROPERTIES
    DESCRIPTION "Support for reading file metadata"
    URL "https://www.kde.org"
    TYPE OPTIONAL)

find_package(KF6NewStuff)
set_package_properties(KF6NewStuff PROPERTIES
    DESCRIPTION "Support for fetching new templates and scripts"
    URL "https://www.kde.org"
    TYPE OPTIONAL)

find_package(KSaneWidgets6)
set_package_properties(KSaneWidgets6 PROPERTIES
    DESCRIPTION "Support for adding scanned images to a collection"
    URL "https://www.kde.org"
    TYPE OPTIONAL)

find_package(KCddb6 5.1)
set_package_properties(KCddb6 PROPERTIES
    DESCRIPTION "Support for CDDB searches"
    URL "https://invent.kde.org/multimedia/libkcddb"
    TYPE OPTIONAL)

find_package(Taglib)
if(TAGLIB_FOUND)
    if(TAGLIB_LIBRARIES)
        # Older taglib
        add_definitions(${TAGLIB_CFLAGS})
        include_directories(${TAGLIB_INCLUDES})
    else()
        set(TAGLIB_LIBRARIES Taglib::Taglib)
    endif()
endif()

find_package(Yaz 3.0)
set_package_properties(Yaz PROPERTIES
    DESCRIPTION "Support for searching z39.50 databases"
    URL "https://www.indexdata.com/resources/software/yaz/"
    TYPE OPTIONAL)
if(Yaz_FOUND)
    include_directories(${Yaz_INCLUDE_DIRS})
endif()

# FindPoppler is part of ECM >= 5.19
find_package(Poppler COMPONENTS Qt6)

find_package(Exempi 2.0)
set_package_properties(Exempi PROPERTIES
    DESCRIPTION "Support for reading PDF/XMP metadata"
    URL "https://libopenraw.freedesktop.org/exempi/"
    TYPE OPTIONAL)
if(Exempi_FOUND)
    include_directories(${Exempi_INCLUDE_DIRS})
endif()

if(ENABLE_BTPARSE)
    find_package(Btparse)
    set_package_properties(Btparse PROPERTIES
        DESCRIPTION "External support for parsing and processing BibTeX data files"
        URL "https://metacpan.org/release/Text-BibTeX"
        TYPE OPTIONAL)
    if(Btparse_FOUND)
        include_directories(${Btparse_INCLUDE_DIRS})
        set(TELLICO_BTPARSE_LIBS ${Btparse_LIBRARIES})
    else()
        set(TELLICO_BTPARSE_LIBS btparse-tellico)
    endif()
endif()

find_package(CDIO)
set_package_properties(CDIO PROPERTIES
    DESCRIPTION "Support for reading cdtext from audio CDs"
    URL "https://www.gnu.org/software/libcdio/"
    TYPE OPTIONAL)
if(CDIO_FOUND)
    include_directories(${CDIO_INCLUDE_DIRS})
endif()

find_package(Csv 3.0)
set_package_properties(Csv PROPERTIES
    DESCRIPTION "External support for reading CSV files"
    URL "http://sourceforge.net/projects/libcsv/"
    TYPE OPTIONAL)
if(Csv_FOUND)
    include_directories(${Csv_INCLUDE_DIRS})
    set(TELLICO_CSV_LIBS ${Csv_LIBRARIES})
else()
    set(TELLICO_CSV_LIBS csv-tellico)
endif()

# webcam uses libv4l, which only works on Linux for now
# Linux 2.6.38 removed the videodev.h header
# libv4l 0.8.3 includes a compat header for videodev.h
if(ENABLE_WEBCAM)
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
        message("WARNING: Webcam support is not available on your platform")
        set(ENABLE_WEBCAM FALSE)
    else()
        pkg_check_modules(LIBV4L libv4l1>=0.6)
        set_package_properties(LIBV4L PROPERTIES
            DESCRIPTION "Support for barcode scanning with a webcam"
            URL "https://hansdegoede.livejournal.com/3636.html"
            TYPE OPTIONAL)
        if(LIBV4L_FOUND)
            if(LIBV4L_VERSION VERSION_LESS "0.8.3" AND CMAKE_SYSTEM_VERSION VERSION_GREATER "2.6.37")
                message("WARNING: libv4l 0.8.3 or later is required for Linux kernel 2.6.38 or later")
                set(ENABLE_WEBCAM FALSE)
            else()
                include_directories(${LIBV4L_INCLUDE_DIR})
            endif()
        else()
            set(ENABLE_WEBCAM FALSE)
        endif()
    endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Wno-long-long -fno-check-new -Woverloaded-virtual")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=declaration-after-statement -Werror=implicit-function-declaration")
endif()

add_definitions(-DQT_STL -DQT_NO_EMIT -DQT_NO_CAST_FROM_ASCII -DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER)
remove_definitions(-DQT_NO_STL -DQT_NO_KEYWORDS -DQT_NO_FOREACH)

include_directories(${LIBXML2_INCLUDE_DIR}
                    ${LIBXSLT_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                    ${Tellico_SOURCE_DIR}/src/config
                    ${Tellico_SOURCE_DIR}/src/3rdparty)

set(TELLICO_DATA_INSTALL_DIR ${KDE_INSTALL_DATADIR}/tellico)

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(xslt)

if(BUILD_DOC)
    add_subdirectory(doc)
endif()

########## Wrap tests results around the tests done within the source

macro_bool_to_01(TAGLIB_FOUND HAVE_TAGLIB)
macro_bool_to_01(Poppler_Qt6_FOUND HAVE_POPPLER)
macro_bool_to_01(Exempi_FOUND HAVE_EXEMPI)
macro_bool_to_01(Yaz_FOUND HAVE_YAZ)
macro_bool_to_01(KSaneWidgets6_FOUND HAVE_KSANE)
macro_bool_to_01(KCddb6_FOUND HAVE_KCDDB)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KABC)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KCAL)
macro_bool_to_01(LIBV4L_FOUND HAVE_V4L)
macro_bool_to_01(KF6NewStuff_FOUND ENABLE_KNEWSTUFF3)
macro_bool_to_01(Btparse_FOUND HAVE_LIBBTPARSE)
macro_bool_to_01(CDIO_FOUND HAVE_CDIO)
macro_bool_to_01(Csv_FOUND HAVE_LIBCSV)
macro_bool_to_01(KF6FileMetaData_FOUND HAVE_KFILEMETADATA)
macro_bool_to_01(Qt6Charts_FOUND HAVE_QCHARTS)
macro_bool_to_01(ENABLE_BTPARSE ENABLE_BTPARSE)
macro_bool_to_01(BUILD_FETCHER_TESTS ENABLE_NETWORK_TESTS)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

########### install files ###############

install(PROGRAMS org.kde.tellico.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES tellico.xml DESTINATION  ${KDE_INSTALL_MIMEDIR})
install(FILES org.kde.tellico.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
ki18n_install(po)

if(BUILD_DOC)
    kdoctools_install(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
