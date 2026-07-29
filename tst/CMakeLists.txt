cmake_minimum_required(VERSION 3.6.3)
project(producerTest)

file(GLOB PRODUCER_TEST_SOURCES *.cpp)

set(INCLUDES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src/")

if (OPEN_SRC_INSTALL_PREFIX)
  find_package(GTest REQUIRED PATHS ${OPEN_SRC_INSTALL_PREFIX})
else()
  find_package(GTest REQUIRED)
endif()

SET(GTEST_LIBNAME GTest::gtest)
if (TARGET GTest::GTest)
  SET(GTEST_LIBNAME GTest::GTest)
endif()

add_executable(${PROJECT_NAME} ${PRODUCER_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}
            KinesisVideoProducer
            ${GTEST_LIBNAME})
add_test(${PROJECT_NAME} ${PROJECT_NAME})

if(BUILD_GSTREAMER_PLUGIN AND NOT WIN32)
  pkg_check_modules(GST_CHECK REQUIRED gstreamer-check-1.0)

  file(GLOB GST_PLUGIN_TEST_SOURCES gstreamer/*.cpp)
  # Compile in the pure region/credential resolution helpers so they can be unit tested directly,
  # rather than only through the runtime-loaded kvssink plugin. This is a dependency-free translation
  # unit (no logging / no static initializers), so linking it into the test binary does not duplicate
  # the plugin's static state (which would crash forked test children at teardown).
  list(APPEND GST_PLUGIN_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/gstreamer/Util/KvsSinkResolution.cpp")
  SET(GST_KVS_PLUGIN_TEST_NAME gstkvsplugintest)

  include_directories("../src/gstreamer")

  add_executable(${GST_KVS_PLUGIN_TEST_NAME} ${GST_PLUGIN_TEST_SOURCES})
  target_link_libraries(${GST_KVS_PLUGIN_TEST_NAME}
              ${GTEST_LIBNAME}
              KinesisVideoProducer
              ${GST_APP_LIBRARIES}
              ${GST_CHECK_LIBRARIES})
  add_test(${GST_KVS_PLUGIN_TEST_NAME} ${GST_KVS_PLUGIN_TEST_NAME})
endif()
