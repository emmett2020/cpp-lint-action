include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.17.0
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(spdlog)
