include(FetchContent)

FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG d418ac22f204b663880a37ebbb82996dc020f603 # 8/12/2024
)

FetchContent_MakeAvailable(tinyxml2)
