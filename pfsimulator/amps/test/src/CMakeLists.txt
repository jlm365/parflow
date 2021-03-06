
include(ParflowTest)

set(SEQUENTIAL_TESTS
  test1
  test4
  test5
  test7
  test8
  test9
  test10
  test17
  )

set(PARALLEL_TESTS
  test1
  test2
  test3
  test4
  test5
  test6
  test7
  test8
  test9
  test10
  test12
  test13
  test14
  test15
  test16
  test17
  )

set(ALL_TESTS ${SEQUENTIAL_TESTS} ${PARALLEL_TESTS})
list(REMOVE_DUPLICATES ALL_TESTS)

foreach(test ${ALL_TESTS})
  add_executable( ${test} ${test}.c)
  target_link_libraries(${test} amps)
endforeach()

foreach(test ${SEQUENTIAL_TESTS})
  pf_add_amps_sequential_test(${test} 1)
endforeach()

if ( ${PARFLOW_AMPS_LAYER} IN_LIST PARFLOW_AMPS_LAYER_REQUIRE_MPI )
  foreach(test ${PARALLEL_TESTS})
    foreach(rank 2 4 8)
      pf_add_amps_parallel_test(${test} ${rank} 1)
    endforeach()
  endforeach()
endif()


