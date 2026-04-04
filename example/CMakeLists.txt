add_executable(test_jwtcpp test_jwtcpp.cpp)
target_link_libraries(test_jwtcpp PRIVATE crypto ssl)


