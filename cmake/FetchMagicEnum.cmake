include(FetchContent)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG v0.9.6
  GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(magic_enum)
