cmake_minimum_required(VERSION 2.8)

project(KinesisVideoProducerC LANGUAGES C)

file(GLOB_RECURSE HEADERS "include/*.h")
file(GLOB_RECURSE SOURCE "source/*.cpp")
file(GLOB_RECURSE SOURCE "source/*.c")

set(INCLUDES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/")
include_directories(${INCLUDES_PATH})
include_directories(${BP_BUILD_INCLUDE_DIRS})
link_directories(${BP_BUILD_LIB})
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCE})
target_link_libraries(${PROJECT_NAME} ${BP_BUILD_LIB_DIRS}
  client heap trace view mkvgen utils curl crypto ssl jsmn)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
          DESTINATION .
                FILES_MATCHING PATTERN "Include.h")

