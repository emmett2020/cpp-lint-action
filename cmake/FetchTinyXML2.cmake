include(FetchContent)

FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG 11.0.0
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(tinyxml2)
