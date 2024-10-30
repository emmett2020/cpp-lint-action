# Dependencies: OpenSSL library

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG 924f214303b860b78350e1e2dfb0521a8724464f  # 4/11/2024
)

FetchContent_MakeAvailable(httplib)
