cmake_minimum_required(VERSION 2.8)

set(KINESIS_VIDEO_DOMAIN com/amazonaws/kinesis/video)

function(kinesis_video_library_setup LIBRARY_NAME)
  set(LIBRARY_HEADERS_DIR include/${KINESIS_VIDEO_DOMAIN}/${LIBRARY_NAME})
  file(GLOB LIBRARY_HEADERS ${LIBRARY_HEADERS_DIR}/*.h)
  file(GLOB LIBRARY_SOURCE src/*.c)
  add_library(${LIBRARY_NAME} STATIC ${LIBRARY_HEADERS} ${LIBRARY_SOURCE})

  target_include_directories(${LIBRARY_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
    $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>)

  target_include_directories(${LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

  if ((NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.4.6"))
      set(CMAKE_CXX_STANDARD 11)
      add_subdirectory(tst)
  endif()
endfunction(kinesis_video_library_setup)

function(kinesis_video_library_install)
  install(TARGETS ${PROJECT_NAME} DESTINATION lib)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION .
    FILES_MATCHING PATTERN "Include.h")
endfunction(kinesis_video_library_install)

link_directories(${BP_LIBFARM_DIR}/lib)
include_directories(${BP_LIBFARM_DIR}/include)

include_directories(${BP_BUILD_INCLUDEDIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(common/include)
add_subdirectory(utils)
add_subdirectory(duration)
add_subdirectory(heap)
add_subdirectory(view)
add_subdirectory(state)
add_subdirectory(mkvgen)
add_subdirectory(trace)
add_subdirectory(client)
add_subdirectory(common)
