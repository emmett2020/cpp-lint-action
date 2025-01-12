# Dependencies:  None
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
  GIT_PROGRESS TRUE
)
message(STATUS "Downloading Catch2")
FetchContent_MakeAvailable(Catch2)
