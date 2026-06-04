# Dependencies: OpenSSL library

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.46.1
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(httplib)
