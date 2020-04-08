cmake_minimum_required(VERSION 3.1)

project(KinesisVideoProducer)

file(GLOB_RECURSE HEADERS "*.h")
file(GLOB_RECURSE SOURCE "*.cpp")

set(INCLUDES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/")

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCE})

# FIXME
# pipeline path doesn't pick up the kinesis video platform independent libs (client, duration, heap, etc.)
# because they are built as static libraries and not shared. Consider changing that package to 
# build shared libraries.
target_link_libraries(${PROJECT_NAME} ${BP_BUILD_LINK_LIBS} client duration heap mkvgen trace state utils view)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)

install(TARGETS ${PROJECT_NAME} DESTINATION lib)

install (DIRECTORY ${CMAKE_SOURCE_DIR}/src/
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        FILES_MATCHING PATTERN "*.h*")
