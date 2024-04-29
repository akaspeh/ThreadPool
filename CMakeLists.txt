cmake_minimum_required(VERSION 3.23)
project(LAB2)

set(CMAKE_CXX_STANDARD 23)

# Add a compile definition for TRACY_ENABLE
add_compile_definitions(TRACY_ENABLE)
# Add your source files
add_executable(LAB2 main.cpp TracyClient.cpp ThreadPool.cpp ThreadPool.h TaskQueue.h)
target_link_libraries(LAB2 PRIVATE ws2_32)
target_link_libraries(LAB2 PRIVATE dbghelp)