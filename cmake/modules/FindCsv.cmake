# Csv_FOUND - system has the csv library
# Csv_INCLUDE_DIRS - the csv include directory
# Csv_LIBRARIES - The libraries needed to use csv

find_path(Csv_INCLUDE_DIRS
    NAMES csv.h
)
find_library(Csv_LIBRARIES NAMES csv)

if(Csv_INCLUDE_DIRS)
  file(READ "${Csv_INCLUDE_DIRS}/csv.h" _csv_header)

  string(REGEX MATCH "define[ \t]+CSV_MAJOR[ \t]+([0-9]+)" _csv_major_version_match "${_csv_header}")
  set(Csv_MAJOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+CSV_MINOR[ \t]+([0-9]+)" _csv_minor_version_match "${_csv_header}")
  set(Csv_MINOR_VERSION "${CMAKE_MATCH_1}")
  string(REGEX MATCH "define[ \t]+CSV_RELEASE[ \t]+([0-9]+)" _csv_release_version_match "${_csv_header}")
  set(Csv_PATCH_VERSION "${CMAKE_MATCH_1}")
  set(Csv_TWEAK_VERSION 0)
  set(Csv_VERSION_COUNT 3)
  set(Csv_VERSION ${Csv_MAJOR_VERSION}.${Csv_MINOR_VERSION}.${Csv_PATCH_VERSION})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Csv REQUIRED_VARS Csv_INCLUDE_DIRS Csv_LIBRARIES VERSION_VAR Csv_VERSION)
endif(Csv_INCLUDE_DIRS)

mark_as_advanced(Csv_INCLUDE_DIRS Csv_LIBRARIES)
