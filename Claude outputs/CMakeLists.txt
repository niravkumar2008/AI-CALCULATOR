cmake_minimum_required(VERSION 3.16)
project(ai_calc CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB CORE core/*.cpp)
add_library(core STATIC ${CORE})
target_compile_options(core PRIVATE -Wall -Wextra)

add_executable(tests tests/tests.cpp)
target_link_libraries(tests core)

if(WIN32)
  add_executable(simulator WIN32 sim/win32_main.cpp)
  target_link_libraries(simulator core gdi32 winhttp windowscodecs ole32 oleaut32 comdlg32)
endif()
