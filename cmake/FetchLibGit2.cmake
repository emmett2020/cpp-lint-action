include(FetchContent)

FetchContent_Declare(
  libgit2
  GIT_REPOSITORY https://github.com/libgit2/libgit2.git
  GIT_TAG c6111ec # 5/11/2024
  GIT_PROGRESS TRUE
)

set(BUILD_SHARED_LIBS ON CACHE BOOL "")
set(USE_NSEC OFF CACHE BOOL "")
set(BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_CLI OFF CACHE BOOL "")
FetchContent_MakeAvailable(libgit2)
