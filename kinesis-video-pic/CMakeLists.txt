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

    set(KINESIS_VIDEO_PIC_SRC ${CMAKE_CURRENT_SOURCE_DIR})

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads)
    set(EXE_LIBRARIES Threads::Threads ${CMAKE_DL_LIBS})

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

    set(PIC_SOURCE_FILES
            src/client/src/AuthIntegration.c
            src/client/src/Callbacks.c
            src/client/src/Client.c
            src/client/src/ClientEvent.c
            src/client/src/ClientState.c
            src/client/src/InputValidator.c
            src/client/src/State.c
            src/client/src/AckParser.c
            src/client/src/Stream.c
            src/client/src/StreamEvent.c
            src/client/src/StreamState.c
            src/client/src/FrameOrderCoordinator.c
            src/heap/src/AivHeap.c
            src/heap/src/Common.c
            src/heap/src/Heap.c
            src/heap/src/HybridHeap.c
            src/heap/src/SystemHeap.c
            src/mkvgen/src/MkvGenerator.c
            src/mkvgen/src/MkvStatics.c
            src/mkvgen/src/NalAdapter.c
            src/mkvgen/src/SpsParser.c
            src/trace/src/TraceProfiler.c
            src/utils/src/Allocators.c
            src/utils/src/Base64.c
            src/utils/src/BitField.c
            src/utils/src/BitReader.c
            src/utils/src/Directory.c
            src/utils/src/DoubleLinkedList.c
            src/utils/src/DynamicLibrary.c
            src/utils/src/Endianness.c
            src/utils/src/FileIo.c
            src/utils/src/HashTable.c
            src/utils/src/Hex.c
            src/utils/src/Mutex.c
            src/utils/src/SingleLinkedList.c
            src/utils/src/StackQueue.c
            src/utils/src/String.c
            src/utils/src/Thread.c
            src/utils/src/Time.c
            src/utils/src/Logger.c
            src/utils/src/Version.c
            src/view/src/ContentView.c)

    set(PIC_TEST_SOURCE_FILES
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/AcksFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/CallbacksAndPressuresFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientApiFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientApiTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/ClientTestFixture.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/IntermittentProducerFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/MockConsumer.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/MockProducer.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StateTransitionFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiFunctionalityScenarioTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiServiceCallsTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamApiTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamDeviceTagsTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamingSession.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamParallelTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamPutGetTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamRecoveryFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamStateTransitionsTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamStoppingFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/StreamTokenRotationTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/client/tst/TokenRotationFunctionalityTest.cpp

            ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapApiFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapApiTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HeapTestFixture.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/heap/tst/HybridHeapTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenApiFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenApiTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/MkvgenTestFixture.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AnnexBNalAdapterTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AnnexBCpdNalAdapterTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/AvccNalAdapterTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/tst/SpsParserTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceApiFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceApiTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/trace/tst/TraceTestFixture.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/BitField.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/BitReader.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/Directory.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/Thread.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/DoubleLinkedList.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/HashTable.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/IntegerToString.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/SingleLinkedList.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StackQueue.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/StringToInteger.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/utils/tst/UtilTestFixture.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewApiFunctionalityTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewApiTest.cpp
            ${KINESIS_VIDEO_PIC_SRC}/src/view/tst/ViewTestFixture.cpp)

    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/client/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/common/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/heap/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/trace/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/mkvgen/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/view/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC}/src/utils/include)
    include_directories(${KINESIS_VIDEO_PIC_SRC})

    add_library(kvspic STATIC ${PIC_SOURCE_FILES})
    target_link_libraries(kvspic)

    if(PIC_TEST)
        set(CMAKE_CXX_STANDARD 11)

        set(GTEST_ROOT $ENV{KVS_GTEST_ROOT})
        find_package(GTest REQUIRED)

        if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            set(EXE_LIBRARIES ${EXE_LIBRARIES} rt)
        endif()

        add_executable(kvspic_test ${PIC_TEST_SOURCE_FILES})
        target_link_libraries(kvspic_test
                kvspic
                GTest::GTest
                GTest::Main
                ${EXE_LIBRARIES})
    endif()
endif()
