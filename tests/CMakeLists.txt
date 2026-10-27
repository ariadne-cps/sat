include(CTest)

add_executable(test_sat_solver test_sat_solver.cpp)
target_link_libraries(test_sat_solver PRIVATE ariadne-sat sat)
add_test(NAME test_sat_solver COMMAND $<TARGET_FILE:test_sat_solver>)
if(WIN32)
    set_tests_properties(test_sat_solver PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:ariadne-sat>"
        TIMEOUT 120
    )
endif()

add_custom_target(tests)
add_dependencies(tests test_sat_solver)
