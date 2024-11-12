cmake_minimum_required(VERSION 3.31)

include(FetchContent)
add_compile_definitions(MACRO_PROJECT_DIR="${PROJECT_SOURCE_DIR}")

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1

FetchContent_MakeAvailable(fmt)

set(CMAKE_CXX_STANDARD 17)

link_libraries(fmt::fmt)
add_executable(main src/main.cpp)