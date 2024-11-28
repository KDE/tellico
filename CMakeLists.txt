# FindPoppler requires CMake >= 3.16
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Tellico)
set(TELLICO_VERSION "4.1+git")

if(DEFINED BUILD_WITH_QT6)
    set(GLOBAL_BUILD_WITH_QT6 ${BUILD_WITH_QT6})
endif()

find_package(ECM REQUIRED NO_MODULE)
# ECM uses the required version to determine whether to build with Qt6
# Instead, use the installed version and assume building with Qt6 unless
# BUILD_WITH_QT6=OFF was passed to cmake
if(NOT DEFINED GLOBAL_BUILD_WITH_QT6 AND
    ECM_VERSION VERSION_GREATER_EQUAL 5.240)
    option(BUILD_WITH_QT6 "Build against Qt 6" ON)
endif()

if(POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()

if(QT_MAJOR_VERSION STREQUAL "6")
    set(QT_MIN_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(CMAKE_CXX_STANDARD 20)
else()
    set(QT_MIN_VERSION "5.12.0")
    set(KF_MIN_VERSION "5.72.0")
    set(CMAKE_CXX_STANDARD 17)
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

option(ENABLE_CDTEXT       "Enable cdtext" TRUE)
option(ENABLE_WEBCAM       "Enable support for webcams" FALSE)
option(BUILD_FETCHER_TESTS "Build tests which verify data sources" FALSE)
option(USE_KHTML           "Build with KHTML rather than QWebEngine" FALSE)
option(ENABLE_BTPARSE      "Build with bibtex importing using btparse, whether internal or external" TRUE)

# btparse does not compile with msvc
if(ENABLE_BTPARSE AND MSVC)
    set(ENABLE_BTPARSE FALSE)
endif()

include(CheckSymbolExists)
check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
check_symbol_exists(strupr "string.h" HAVE_STRUPR)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Widgets
    Xml
    DBus
    Test
    Network
    PrintSupport
)

set(KF_MAJOR_VERSION ${QT_MAJOR_VERSION})
find_package(KF${KF_MAJOR_VERSION} REQUIRED COMPONENTS
    Archive
    Codecs
    Completion
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
    Sonnet
    TextWidgets
    WidgetsAddons
    XmlGui
)

if(NOT USE_KHTML)
    find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG OPTIONAL_COMPONENTS
        WebEngineWidgets
    )
    set_package_properties(Qt${QT_MAJOR_VERSION}WebEngineWidgets PROPERTIES
    DESCRIPTION "Building with QWebEngine is preferrable to KHTML"
    TYPE OPTIONAL)
endif()
if(USE_KHTML OR NOT Qt${QT_MAJOR_VERSION}WebEngineWidgets_FOUND)
    set(USE_KHTML ON)
    find_package(KF${KF_MAJOR_VERSION}KHtml REQUIRED NO_MODULE)
    set_package_properties(KF${KF_MAJOR_VERSION}Html PROPERTIES
    DESCRIPTION "Building with QWebEngine is preferrable to KHTML")
endif()

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG OPTIONAL_COMPONENTS
    Charts
)

include(MacroBoolTo01)

find_package(Gettext REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

find_package(KF${KF_MAJOR_VERSION}FileMetaData)
set_package_properties(KF${KF_MAJOR_VERSION}FileMetaData PROPERTIES
    DESCRIPTION "Support for reading file metadata"
    URL "https://www.kde.org"
    TYPE OPTIONAL)

find_package(KF${KF_MAJOR_VERSION}NewStuff)
set_package_properties(KF${KF_MAJOR_VERSION}NewStuff PROPERTIES
    DESCRIPTION "Support for fetching new templates and scripts"
    URL "https://www.kde.org"
    TYPE OPTIONAL)

if(KF_MAJOR_VERSION STREQUAL "5")
    set(KSANE_TARGET_NAME KF5::Sane)
    find_package(KF5Sane)
else()
    set(KSANE_TARGET_NAME KSaneWidgets6)
    find_package(KSaneWidgets6)
endif()
set_package_properties(KSANE_TARGET_NAME PROPERTIES
    DESCRIPTION "Support for adding scanned images to a collection"
    URL "https://www.kde.org"
    TYPE OPTIONAL)

if(KF_MAJOR_VERSION STREQUAL "5")
    set(KCDDB_TARGET_NAME KF5::Cddb)
    find_package(KF5Cddb 5.1)
else()
    set(KCDDB_TARGET_NAME KCddb6)
    find_package(KCddb6 5.1)
endif()
set_package_properties(${KCDDB_TARGET_NAME} PROPERTIES
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

find_package(Yaz 2.0)
set_package_properties(Yaz PROPERTIES
    DESCRIPTION "Support for searching z39.50 databases"
    URL "https://www.indexdata.com/resources/software/yaz/"
    TYPE OPTIONAL)
if(Yaz_FOUND)
    include_directories(${Yaz_INCLUDE_DIRS})
endif()

# FindPoppler is part of ECM >= 5.19
find_package(Poppler COMPONENTS Qt${QT_MAJOR_VERSION})

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
endif()

add_definitions(-DQT_STL -DQT_NO_CAST_FROM_ASCII -DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER)
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

macro_bool_to_01(TAGLIB_FOUND HAVE_TAGLIB)
macro_bool_to_01(Poppler_Qt${QT_MAJOR_VERSION}_FOUND HAVE_POPPLER)
macro_bool_to_01(Exempi_FOUND HAVE_EXEMPI)
macro_bool_to_01(Yaz_FOUND HAVE_YAZ)
macro_bool_to_01(${KSANE_TARGET_NAME}_FOUND HAVE_KSANE)
macro_bool_to_01(Libkcddb_FOUND HAVE_OLD_KCDDB)
macro_bool_to_01(${KCDDB_TARGET_NAME}_FOUND HAVE_KCDDB)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KABC)
macro_bool_to_01(KDEPIMLIBS_FOUND HAVE_KCAL)
macro_bool_to_01(LIBV4L_FOUND HAVE_V4L)
macro_bool_to_01(KF${KF_MAJOR_VERSION}NewStuff_FOUND ENABLE_KNEWSTUFF3)
macro_bool_to_01(Btparse_FOUND HAVE_LIBBTPARSE)
macro_bool_to_01(CDIO_FOUND HAVE_CDIO)
macro_bool_to_01(Csv_FOUND HAVE_LIBCSV)
macro_bool_to_01(KF${KF_MAJOR_VERSION}FileMetaData_FOUND HAVE_KFILEMETADATA)
macro_bool_to_01(Qt${QT_MAJOR_VERSION}Charts_FOUND HAVE_QCHARTS)
macro_bool_to_01(ENABLE_BTPARSE ENABLE_BTPARSE)
macro_bool_to_01(BUILD_FETCHER_TESTS ENABLE_NETWORK_TESTS)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

########### install files ###############

install(PROGRAMS org.kde.tellico.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES tellico.xml DESTINATION  ${KDE_INSTALL_MIMEDIR})
install(FILES org.kde.tellico.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
ki18n_install(po)
# some translations require KF6, skip those when creating docs
if(NOT KF_MAJOR_VERSION STREQUAL "5")
    kdoctools_install(po)
else()
    # taken from kdoctools cmake macro
    file(GLOB lang_dirs "po/*")
    # remove the non-backwards compatible translations
    list(FILTER lang_dirs EXCLUDE REGEX "po/sl")
    if(NOT KDE_INSTALL_DOCBUNDLEDIR)
        if(HTML_INSTALL_DIR)
            set(KDE_INSTALL_DOCBUNDLEDIR ${HTML_INSTALL_DIR})
        else()
            set(KDE_INSTALL_DOCBUNDLEDIR share/doc/HTML)
        endif()
    endif()
    foreach(lang_dir ${lang_dirs})
        get_filename_component(lang ${lang_dir} NAME)

        file(GLOB_RECURSE docbooks RELATIVE "${lang_dir}" "${lang_dir}/docs/*.docbook")
        foreach(docbook ${docbooks})
            string(REGEX MATCH "^docs/(.*)/index.docbook" match ${docbook})
            if(match)
                kdoctools_create_handbook("${lang_dir}/${docbook}"
                    INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/${lang}
                    SUBDIR ${CMAKE_MATCH_1}
                )
            endif()
        endforeach()
    endforeach()
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
