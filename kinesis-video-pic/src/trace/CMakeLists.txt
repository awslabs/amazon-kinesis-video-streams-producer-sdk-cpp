cmake_minimum_required(VERSION 2.8)

project(trace)
kinesis_video_library_setup(${PROJECT_NAME})
target_link_libraries(trace utils)
kinesis_video_library_install()
