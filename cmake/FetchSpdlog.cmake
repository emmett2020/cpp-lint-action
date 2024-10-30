include(FetchContent)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG 6192537d08e47c34cbf78d8363d74f8c74b23d69 # 2/11/2024
)

FetchContent_MakeAvailable(spdlog)
