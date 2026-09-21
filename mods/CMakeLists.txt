# Native adaptations are exercised with the real guest callback/return driver.
add_executable(display_tests ../tests/display_tests.cpp ../native/display.c
  ${POP_ROOT}/runtime/tests/stub_recomp_call.cpp)
target_link_libraries(display_tests PRIVATE recomp_platform recomp_runtime recomp_dx)
target_compile_options(display_tests PRIVATE ${POP_WARN_HOST})
pop_optimize(display_tests 1)
pop_test_binary(display_tests)
add_test(NAME display_tests COMMAND display_tests WORKING_DIRECTORY ${RECOMP_GAME_DIR})
add_executable(input_tests ../tests/input_tests.cpp ../native/input.c
  ${POP_ROOT}/runtime/tests/stub_recomp_call.cpp)
target_link_libraries(input_tests PRIVATE recomp_platform recomp_runtime recomp_dx)
target_compile_options(input_tests PRIVATE ${POP_WARN_HOST})
pop_optimize(input_tests 1)
pop_test_binary(input_tests)
add_test(NAME input_tests COMMAND input_tests WORKING_DIRECTORY ${RECOMP_GAME_DIR})
set_tests_properties(input_tests display_tests PROPERTIES LABELS "nogame")
if(TARGET mods_tests)
  # The generated game calls a host service; isolated mod suites use its
  # weak no-op default while retaining their own strong service test doubles.
  target_sources(mods_tests PRIVATE ${POP_ROOT}/dx/host_api.cpp)
  add_dependencies(check_binaries mods_tests)
endif()
