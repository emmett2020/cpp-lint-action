include(FetchContent)

FetchContent_Declare(
  range-v3
  GIT_REPOSITORY https://github.com/ericniebler/range-v3.git
  GIT_TAG 7e6f34b1e820fb8321346888ef0558a0ec842b8e # 2/11/2024
)

FetchContent_MakeAvailable(range-v3)
