# Mainly stolen from FindXMLRPC.cmake
#  FIND_PACKAGE(Yaz REQUIRED)

# First find the config script from which to obtain other values.
FIND_PROGRAM(YAZ_CONFIG NAMES yaz-config)

# Check whether we found anything.
IF(YAZ_CONFIG)
  SET(YAZ_FOUND 1)
ELSE(YAZ_CONFIG)
  SET(YAZ_FOUND 0)
ENDIF(YAZ_CONFIG)

# Lookup the include directories needed
IF(YAZ_FOUND)
  # Use the newer EXECUTE_PROCESS command if it is available.
  IF(COMMAND EXECUTE_PROCESS)
    EXECUTE_PROCESS(
      COMMAND ${YAZ_CONFIG} --cflags
      OUTPUT_VARIABLE YAZ_CONFIG_CFLAGS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE YAZ_CONFIG_RESULT
      )
  ELSE(COMMAND EXECUTE_PROCESS)
    EXEC_PROGRAM(${YAZ_CONFIG} ARGS "--cflags"
      OUTPUT_VARIABLE YAZ_CONFIG_CFLAGS
      RETURN_VALUE YAZ_CONFIG_RESULT
      )
  ENDIF(COMMAND EXECUTE_PROCESS)

  # Parse the include flags.
  IF("${YAZ_CONFIG_RESULT}" MATCHES "^0$")
    # Convert the compile flags to a CMake list.
    STRING(REGEX REPLACE " +" ";"
      YAZ_CONFIG_CFLAGS "${YAZ_CONFIG_CFLAGS}")

    # Look for -I options.
    SET(YAZ_INCLUDE_DIRS)
    FOREACH(flag ${YAZ_CONFIG_CFLAGS})
      IF("${flag}" MATCHES "^-I")
        STRING(REGEX REPLACE "^-I" "" DIR "${flag}")
        FILE(TO_CMAKE_PATH "${DIR}" DIR)
        SET(YAZ_INCLUDE_DIRS ${YAZ_INCLUDE_DIRS} "${DIR}")
      ENDIF("${flag}" MATCHES "^-I")
    ENDFOREACH(flag)
  ELSE("${YAZ_CONFIG_RESULT}" MATCHES "^0$")
    MESSAGE("Error running ${YAZ_CONFIG}: [${YAZ_CONFIG_RESULT}]")
    SET(_FOUND 0)
  ENDIF("${YAZ_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(YAZ_FOUND)

# Lookup the libraries needed.
IF(YAZ_FOUND)
  # Use the newer EXECUTE_PROCESS command if it is available.
  IF(COMMAND EXECUTE_PROCESS)
    EXECUTE_PROCESS(
      COMMAND ${YAZ_CONFIG} --libs
      OUTPUT_VARIABLE YAZ_CONFIG_LIBS
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE YAZ_CONFIG_RESULT
      )
  ELSE(COMMAND EXECUTE_PROCESS)
    EXEC_PROGRAM(${YAZ_CONFIG} ARGS "--libs"
      OUTPUT_VARIABLE YAZ_CONFIG_LIBS
      RETURN_VALUE YAZ_CONFIG_RESULT
      )
  ENDIF(COMMAND EXECUTE_PROCESS)

  # Parse the library names and directories.
  IF("${YAZ_CONFIG_RESULT}" MATCHES "^0$")
    STRING(REGEX REPLACE " +" ";"
      YAZ_CONFIG_LIBS "${YAZ_CONFIG_LIBS}")

    # Look for -L flags for directories and -l flags for library names.
    SET(YAZ_LIBRARY_DIRS)
    SET(YAZ_LIBRARY_NAMES)
    FOREACH(flag ${YAZ_CONFIG_LIBS})
      IF("${flag}" MATCHES "^-L")
        STRING(REGEX REPLACE "^-L" "" DIR "${flag}")
        FILE(TO_CMAKE_PATH "${DIR}" DIR)
        SET(YAZ_LIBRARY_DIRS ${YAZ_LIBRARY_DIRS} "${DIR}")
      ELSEIF("${flag}" MATCHES "^-l")
        STRING(REGEX REPLACE "^-l" "" NAME "${flag}")
        SET(YAZ_LIBRARIES ${YAZ_LIBRARIES} "${NAME}")
      ENDIF("${flag}" MATCHES "^-L")
    ENDFOREACH(flag)
  ELSE("${YAZ_CONFIG_RESULT}" MATCHES "^0$")
    MESSAGE("Error running ${YAZ_CONFIG}: [${YAZ_CONFIG_RESULT}]")
    SET(YAZ_FOUND 0)
  ENDIF("${YAZ_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(YAZ_FOUND)

# Report the results.
IF(NOT YAZ_FOUND)
  SET(YAZ_DIR_MESSAGE
    " was not found.")
  IF(NOT YAZ_FIND_QUIETLY)
    MESSAGE(STATUS "${YAZ_DIR_MESSAGE}")
  ELSE(NOT YAZ_FIND_QUIETLY)
    IF(YAZ_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "${YAZ_DIR_MESSAGE}")
    ENDIF(YAZ_FIND_REQUIRED)
  ENDIF(NOT YAZ_FIND_QUIETLY)
ENDIF(NOT YAZ_FOUND)
