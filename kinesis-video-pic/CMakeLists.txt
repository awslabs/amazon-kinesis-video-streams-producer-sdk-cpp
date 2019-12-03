cmake_minimum_required(VERSION 2.8)

if(BP_INCLUDE_DIRS)
  project(PlatformIndependentNativeRepository C)

  include_directories(${BP_TESTBUILD_LIBFARM}/include)
  link_directories(${BP_TESTBUILD_LIBFARM}/lib)
  link_directories(${BP_TESTBUILD_LIB})

  include_directories(${BP_INCLUDE_DIRS})

  # Required for compilation of AL2012
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -lrt -lpthread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ldl -lrt -lpthread")
  endif()

  get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

  # Include source base directory to allow absolute header paths
  include_directories(${CMAKE_SOURCE_DIR})
  enable_testing()
  add_subdirectory(src)
else()
  project(pic_project)

  if(NOT KINESIS_VIDEO_OPEN_SOURCE_SRC)
    if(DEFINED ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
      set(KINESIS_VIDEO_OPEN_SOURCE_SRC $ENV{KINESIS_VIDEO_OPEN_SOURCE_SRC})
    else()
      set(KINESIS_VIDEO_OPEN_SOURCE_SRC
          "${CMAKE_CURRENT_SOURCE_DIR}/../kinesis-video-native-build/downloads/local"
      )
    endif()
  endif()

  set(KINESIS_VIDEO_PIC_SRC ${CMAKE_CURRENT_SOURCE_DIR})

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads)

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

  file(GLOB PIC_CLIENT_SOURCE_FILES "src/client/src/*.c")

  file(GLOB PIC_DURATION_SOURCE_FILES "src/duration/src/*.c")

  file(GLOB PIC_HEAP_SOURCE_FILES "src/heap/src/*.c")

  file(GLOB PIC_MKVGEN_SOURCE_FILES "src/mkvgen/src/*.c")

  file(GLOB PIC_STATE_SOURCE_FILES "src/state/src/*.c")

  file(GLOB PIC_TRACE_SOURCE_FILES "src/trace/src/*.c")

  file(GLOB PIC_UTILS_SOURCE_FILES "src/utils/src/*.c")

  file(GLOB PIC_VIEW_SOURCE_FILES "src/view/src/*.c")

  file(
    GLOB
    PIC_TEST_SOURCE_FILES
    "src/client/tst/*.cpp"
    "src/duration/tst/*.cpp"
    "src/heap/tst/*.cpp"
    "src/mkvgen/tst/*.cpp"
    "src/state/tst/*.cpp"
    "src/trace/tst/*.cpp"
    "src/utils/tst/*.cpp"
    "src/view/tst/*.cpp")

  file(GLOB PIC_HEADERS "${KINESIS_VIDEO_PIC_SRC}/src/*/include")

  include_directories(${PIC_HEADERS})
  include_directories(${KINESIS_VIDEO_PIC_SRC})

  add_library(
    kvspic STATIC
    ${PIC_CLIENT_SOURCE_FILES}
    ${PIC_DURATION_SOURCE_FILES}
    ${PIC_HEAP_SOURCE_FILES}
    ${PIC_MKVGEN_SOURCE_FILES}
    ${PIC_STATE_SOURCE_FILES}
    ${PIC_TRACE_SOURCE_FILES}
    ${PIC_UTILS_SOURCE_FILES}
    ${PIC_VIEW_SOURCE_FILES})
  target_link_libraries(kvspic ${CMAKE_DL_LIBS} Threads::Threads)
  if(UNIX AND NOT APPLE)
    # rt needed for clock_gettime
    target_link_libraries(kvspic rt)
  endif()

  add_library(kvspicClient STATIC ${PIC_CLIENT_SOURCE_FILES})
  target_link_libraries(kvspicClient)

  add_library(kvspicState STATIC ${PIC_STATE_SOURCE_FILES})
  target_link_libraries(kvspicState)

  add_library(kvspicUtils STATIC ${PIC_UTILS_SOURCE_FILES})
  target_link_libraries(kvspicUtils ${CMAKE_DL_LIBS} Threads::Threads)
  if(UNIX AND NOT APPLE)
    # rt needed for clock_gettime
    target_link_libraries(kvspicUtils rt)
  endif()

  if(BUILD_TEST)
    set(CMAKE_CXX_STANDARD 11)

    set(GTEST_ROOT ${KINESIS_VIDEO_OPEN_SOURCE_SRC})
    find_package(GTest REQUIRED)

    add_executable(kvspic_test ${PIC_TEST_SOURCE_FILES})
    target_link_libraries(kvspic_test kvspic GTest::GTest GTest::Main)
  endif()
endif()
