include(CTest)

function(add_test_executable target_name)
  add_executable(${target_name} ${ARGN})
  # target_link_libraries(${target_name} PRIVATE benet)
endfunction()

function(add_ctest_executable target_name)
  add_executable(${target_name} ${ARGN})
  # target_link_libraries(${target_name} PRIVATE benet)
  add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()
 
add_test_executable(keylogger keylogger.cpp) 


