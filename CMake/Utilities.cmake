# only fetch target repo for add_subdirectory later
function(fetch_repo lib_name)
  set(supported_libs
      kvspic
      kvscproducer)
  list(FIND supported_libs ${lib_name} index)
  if(${index} EQUAL -1)
    message(WARNING "${lib_name} is not supported for fetch_repo")
    return()
  endif()

  if (WIN32 OR NOT PARALLEL_BUILD)
    set(PARALLEL_BUILD "")  # No parallel build for Windows
  else()
    set(PARALLEL_BUILD "--parallel")  # Enable parallel builds for Unix-like systems
  endif()

  # build library
  configure_file(
    ./CMake/Dependencies/lib${lib_name}-CMakeLists.txt
    ${DEPENDENCY_DOWNLOAD_PATH}/lib${lib_name}/CMakeLists.txt COPYONLY)
  execute_process(
    COMMAND ${CMAKE_COMMAND}
            ${CMAKE_GENERATOR} .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${DEPENDENCY_DOWNLOAD_PATH}/lib${lib_name})
  if(result)
    message(FATAL_ERROR "CMake step for lib${lib_name} failed: ${result}")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . ${PARALLEL_BUILD}
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${DEPENDENCY_DOWNLOAD_PATH}/lib${lib_name})
  if(result)
    message(FATAL_ERROR "CMake step for lib${lib_name} failed: ${result}")
  endif()
endfunction()

# build library from source
function(build_dependency lib_name)
  set(supported_libs
      log4cplus)
  list(FIND supported_libs ${lib_name} index)
  if(${index} EQUAL -1)
    message(WARNING "${lib_name} is not supported to build from source")
    return()
  endif()

  set(target_found NOTFOUND)

  set(lib_file_name ${lib_name})

  find_library(target_found
      NAMES ${lib_file_name}
      PATHS ${OPEN_SRC_INSTALL_PREFIX}/lib
      NO_DEFAULT_PATH)

  if(target_found)
    message(STATUS "${lib_name} already built")
    return()
  endif()

  # anything after lib_name(${ARGN}) are assumed to be arguments passed over to
  # library building cmake.
  set(build_args ${ARGN})

  file(REMOVE_RECURSE ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib${lib_name})

  if (WIN32 OR NOT PARALLEL_BUILD)
    set(PARALLEL_BUILD "")  # No parallel build for Windows
  else()
    set(PARALLEL_BUILD "--parallel")  # Enable parallel builds for Unix-like systems
  endif()

  # build library
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Dependencies/lib${lib_name}-CMakeLists.txt
    ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib${lib_name}/CMakeLists.txt COPYONLY)
  execute_process(
    COMMAND ${CMAKE_COMMAND} ${build_args}
            -DOPEN_SRC_INSTALL_PREFIX=${OPEN_SRC_INSTALL_PREFIX} -G
            ${CMAKE_GENERATOR} .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib${lib_name})
  if(result)
    message(FATAL_ERROR "CMake step for lib${lib_name} failed: ${result}")
  endif()
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build . ${PARALLEL_BUILD}
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib${lib_name})
  if(result)
    message(FATAL_ERROR "CMake step for lib${lib_name} failed: ${result}")
  endif()

  file(REMOVE_RECURSE ${KINESIS_VIDEO_OPEN_SOURCE_SRC}/lib${lib_name})
endfunction()

function(enableSanitizer SANITIZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fsanitize=${SANITIZER} -fno-omit-frame-pointer" PARENT_SCOPE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -fsanitize=${SANITIZER} -fno-omit-frame-pointer -fno-optimize-sibling-calls" PARENT_SCOPE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${SANITIZER}" PARENT_SCOPE)
endfunction()
