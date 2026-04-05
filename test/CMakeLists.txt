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

add_test_executable(checkkbd checkkbd.cpp) 
target_link_libraries(checkkbd PRIVATE udev)

add_executable(test_jwtcpp test_jwtcpp.cpp)
target_link_libraries(test_jwtcpp PRIVATE crypto ssl)


