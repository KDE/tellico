# CSV_FOUND - system has the csv library
# CSV_INCLUDE_DIR - the csv include directory
# CSV_LIBRARIES - The libraries needed to use csv

#in cache already
if(CSV_INCLUDE_DIR AND CSV_LIBRARIES)
  set(CSV_FIND_QUIETLY TRUE)
endif(CSV_INCLUDE_DIR AND CSV_LIBRARIES)

find_path(CSV_INCLUDE_DIR
    NAMES csv.h
)
find_library(CSV_LIBRARIES NAMES csv)

if(CSV_INCLUDE_DIR)
  file(READ "${CSV_INCLUDE_DIR}/csv.h" _csv_header)

  string(REGEX MATCH "define[ \t]+CSV_MAJOR[ \t]+([0-9]+)" _csv_major_version_match "${_csv_header}")
  set(CSV_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+CSV_MINOR[ \t]+([0-9]+)" _csv_minor_version_match "${_csv_header}")
  set(CSV_MINOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+CSV_RELEASE[ \t]+([0-9]+)" _csv_release_version_match "${_csv_header}")
  set(CSV_PATCH_VERSION "${CMAKE_MATCH_1}")
  set(CSV_TWEAK_VERSION 0)
  set(CSV_VERSION_COUNT 3)
  set(CSV_VERSION ${CSV_MAJOR_VERSION}.${CSV_MINOR_VERSION}.${CSV_PATCH_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Csv REQUIRED_VARS CSV_INCLUDE_DIR CSV_LIBRARIES VERSION_VAR CSV_VERSION)
endif(CSV_INCLUDE_DIR)

mark_as_advanced(CSV_INCLUDE_DIR CSV_LIBRARIES)
