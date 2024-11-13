cmake_minimum_required(VERSION 3.31)

include(FetchContent)
add_compile_definitions(MACRO_PROJECT_DIR="${PROJECT_SOURCE_DIR}")

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1

FetchContent_MakeAvailable(fmt)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG main # Important: Specify a version or a commit hash here.
)
FetchContent_MakeAvailable(ftxui)

set(CMAKE_CXX_STANDARD 17)
if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()


link_libraries(spdlog::spdlog_header_only)
link_libraries(fmt::fmt)

add_executable(main src/main.cpp)

add_executable(window src/window.cpp)
target_link_libraries(window
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component # Not needed for this example.
)