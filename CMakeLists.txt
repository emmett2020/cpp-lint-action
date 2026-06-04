cmake_minimum_required(VERSION 3.13)

# Read and watch VERSION file changes
set(VERSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${VERSION_FILE})
file(READ "${VERSION_FILE}" CPP_LINT_ACTION_VERSION)
string(STRIP "${CPP_LINT_ACTION_VERSION}" CPP_LINT_ACTION_VERSION)

project(cpp_lint_action VERSION ${CPP_LINT_ACTION_VERSION})

set(CMAKE_CXX_STANDARD            20)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(cpp_lint_action_root ${CMAKE_CURRENT_SOURCE_DIR})
set(config_dir           ${cpp_lint_action_root}/src/configs)
set(src_dir              ${cpp_lint_action_root}/src)
set(generated_config_dir ${CMAKE_CURRENT_BINARY_DIR}/generated/configs)

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

find_package(Boost REQUIRED CONFIG COMPONENTS filesystem process regex program_options)
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

set(cpp_lint_action_libgit2_target "")
if(PkgConfig_FOUND)
  pkg_check_modules(LIBGIT2 QUIET IMPORTED_TARGET libgit2)
endif()
if(TARGET PkgConfig::LIBGIT2)
  set(cpp_lint_action_libgit2_target PkgConfig::LIBGIT2)
else()
  find_package(libgit2 CONFIG QUIET)
  if(TARGET libgit2::libgit2package)
    set(cpp_lint_action_libgit2_target libgit2::libgit2package)
  else()
    find_path(LIBGIT2_INCLUDE_DIR git2.h)
    find_library(LIBGIT2_LIBRARY NAMES git2)
    if(NOT LIBGIT2_INCLUDE_DIR OR NOT LIBGIT2_LIBRARY)
      message(FATAL_ERROR "Could not find libgit2. Install libgit2-dev or provide libgit2.pc.")
    endif()
  endif()
endif()

configure_file(${config_dir}/version.h.in ${generated_config_dir}/version.h)

FILE(GLOB_RECURSE dep_files "${src_dir}/github/*.cpp"
                            "${src_dir}/tools/*.cpp"
                            "${src_dir}/utils/*.cpp"
                            "${src_dir}/program_options.cpp"
                            "${src_dir}/context.cpp"
)

add_library(dep_obj OBJECT ${dep_files})
target_include_directories(dep_obj
  PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${magic_enum_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${src_dir}
)
if(LIBGIT2_INCLUDE_DIR)
  target_include_directories(dep_obj PUBLIC ${LIBGIT2_INCLUDE_DIR})
endif()
target_link_libraries(dep_obj
  PUBLIC
    Boost::filesystem
    Boost::process
    Boost::regex
    Boost::program_options
    spdlog
    range-v3
    httplib
    nlohmann_json
    tinyxml2
    magic_enum
    Threads::Threads
)
if(cpp_lint_action_libgit2_target)
  target_link_libraries(dep_obj PUBLIC ${cpp_lint_action_libgit2_target})
else()
  target_link_libraries(dep_obj PUBLIC ${LIBGIT2_LIBRARY})
endif()

add_executable(cpp-lint-action "${src_dir}/main.cpp")
target_link_libraries(cpp-lint-action PRIVATE dep_obj)

IF (BUILD_TESTING)
  add_subdirectory(tests)
ENDIF()
