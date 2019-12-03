cmake_minimum_required(VERSION 2.8)

project(client)
kinesis_video_library_setup(${PROJECT_NAME})
target_link_libraries(client mkvgen heap view utils state)
kinesis_video_library_install()
