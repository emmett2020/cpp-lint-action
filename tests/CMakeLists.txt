FILE(GLOB_RECURSE test_files "${cpp_lint_action_root}/tests/*.cpp")
add_executable(test ${test_files} $<TARGET_OBJECTS:dep_obj>)
target_link_libraries(test PRIVATE Catch2::Catch2)

