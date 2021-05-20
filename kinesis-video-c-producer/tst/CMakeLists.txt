cmake_minimum_required(VERSION 2.8)

project (producerTest)

set(CMAKE_CXX_STANDARD 11)
set(KINESIS_VIDEO_PRODUCER_C_SRC "${CMAKE_CURRENT_SOURCE_DIR}/..")

if (OPEN_SRC_INSTALL_PREFIX)
  find_package(GTest REQUIRED PATHS ${OPEN_SRC_INSTALL_PREFIX})
else()
  find_package(GTest REQUIRED)
endif()

SET(GTEST_LIBNAME GTest::gtest)
if (TARGET GTest::GTest)
  SET(GTEST_LIBNAME GTest::GTest)
endif()

include_directories(${KINESIS_VIDEO_PRODUCER_C_SRC})

file(GLOB PRODUCER_TEST_SOURCE_FILES "*.cpp")

add_executable(producer_test ${PRODUCER_TEST_SOURCE_FILES})
target_link_libraries(producer_test cproducer ${GTEST_LIBNAME})
