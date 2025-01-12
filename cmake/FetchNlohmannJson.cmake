include(FetchContent)

FetchContent_Declare(
  nlohmann-json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(nlohmann-json)
