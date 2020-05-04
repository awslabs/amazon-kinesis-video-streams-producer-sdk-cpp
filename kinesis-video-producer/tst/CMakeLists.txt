cmake_minimum_required(VERSION 3.1)
project(producerTest)

file(GLOB PRODUCER_TEST_SOURCES *.cpp)

set(INCLUDES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src/")

set(GTEST_ROOT $ENV{KVS_GTEST_ROOT})
find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} ${PRODUCER_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}
            GTest::GTest
            KinesisVideoProducer)
add_test(${PROJECT_NAME} ${PROJECT_NAME})

if(BUILD_GSTREAMER_PLUGIN)
  pkg_check_modules(GST_CHECK REQUIRED gstreamer-check-1.0)

  file(GLOB GST_PLUGIN_TEST_SOURCES gstreamer/*.cpp)
  SET(GST_KVS_PLUGIN_TEST_NAME gstkvsplugintest)

  include_directories("../src/gstreamer")

  add_executable(${GST_KVS_PLUGIN_TEST_NAME} ${GST_PLUGIN_TEST_SOURCES})
  target_link_libraries(${GST_KVS_PLUGIN_TEST_NAME}
              GTest::GTest
              KinesisVideoProducer
              ${GST_APP_LIBRARIES}
              ${GST_CHECK_LIBRARIES})
  add_test(${GST_KVS_PLUGIN_TEST_NAME} ${GST_KVS_PLUGIN_TEST_NAME})
endif()
