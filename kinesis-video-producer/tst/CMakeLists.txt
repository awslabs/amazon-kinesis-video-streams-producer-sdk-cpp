cmake_minimum_required(VERSION 3.1)

project (producerTest)

file (GLOB ProducerTestSources *.cpp)

set(INCLUDES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src/")

include_directories(${INCLUDES_PATH})
include_directories(${BP_BUILD_INCLUDE_DIRS})
link_directories(${BP_BUILD_LIB})

add_executable(${PROJECT_NAME} ${ProducerTestSources})
target_link_libraries(${PROJECT_NAME} KinesisVideoProducer ${BP_TEST_BUILD_LINK_LIBS})
target_link_libraries(${PROJECT_NAME} gtest client heap trace view mkvgen utils)

add_test(${PROJECT_NAME} ${PROJECT_NAME})
