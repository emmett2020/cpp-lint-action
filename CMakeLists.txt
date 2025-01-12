cmake_minimum_required(VERSION 3.13)

# Read and watch VERSION file changes
set(VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${VERSION_FILE})
file(READ "${VERSION_FILE}" CPP_LINT_ACTION_VERSION)
string(STRIP "${CPP_LINT_ACTION_VERSION}" CPP_LINT_ACTION_VERSION)

project(cpp_lint_action VERSION ${CPP_LINT_ACTION_VERSION})

set(CMAKE_CXX_STANDARD             20)
set(CXX_STANDARD_REQUIRED          true)
set(CMAKE_EXPORT_COMPILE_COMMANDS  on)
set(CMAKE_BUILD_TYPE               Debug)

set(cpp_lint_action_root ${CMAKE_CURRENT_SOURCE_DIR})
set(config_dir  ${cpp_lint_action_root}/src/configs)
set(src_dir     ${cpp_lint_action_root}/src)

include(${cpp_lint_action_root}/cmake/FetchSpdlog.cmake)
include(${cpp_lint_action_root}/cmake/FetchRangeV3.cmake)
include(${cpp_lint_action_root}/cmake/FetchHttpLib.cmake)
include(${cpp_lint_action_root}/cmake/FetchNlohmannJson.cmake)
include(${cpp_lint_action_root}/cmake/FetchMagicEnum.cmake)
include(${cpp_lint_action_root}/cmake/FetchTinyXML2.cmake)

option(BUILD_TESTING "Build tests" ON)
IF (BUILD_TESTING)
  include(${cpp_lint_action_root}/cmake/FetchCatch2.cmake)
ENDIF()

OPTION (ENABLE_CODE_COVERAGE "Enable coverage" OFF)
IF(ENABLE_CODE_COVERAGE)
  message(STATUS ENABLE_CODE_COVERAGE=${ENABLE_CODE_COVERAGE})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-abs-path -g -O0")
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} --coverage -fprofile-abs-path -g -O0")
ENDIF()

find_package(Boost REQUIRED CONFIG COMPONENTS filesystem system regex program_options)
find_package(libgit2 REQUIRED CONFIG)

configure_file(${config_dir}/version.h.in ${config_dir}/version.h)

include_directories(${Boost_INCLUDE_DIRS}
                    ${magic_enum_SOURCE_DIR}/include
                    ${libgit2_INCLUDE_DIRS}
                    ${src_dir})

link_libraries(${Boost_LIBRARIES}
               spdlog
               range-v3
               httplib
               nlohmann_json
               tinyxml2
               magic_enum
               git2)

FILE(GLOB_RECURSE dep_files "${src_dir}/github/*.cpp"
                            "${src_dir}/tools/*.cpp"
                            "${src_dir}/utils/*.cpp"
                            "${src_dir}/program_options.cpp"
                            "${src_dir}/context.cpp"
)

add_library(dep_obj OBJECT ${dep_files})
add_executable(cpp-lint-action $<TARGET_OBJECTS:dep_obj> "${src_dir}/main.cpp")

IF (BUILD_TESTING)
  add_subdirectory(tests)
ENDIF()
