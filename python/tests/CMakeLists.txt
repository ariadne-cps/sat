execute_process(
    COMMAND "${Python3_EXECUTABLE}" -m pytest --version
    RESULT_VARIABLE PYTEST_result
    OUTPUT_QUIET ERROR_QUIET
)
if(PYTEST_result EQUAL 0)
    set(PYTHON_TESTS ON)
    message(STATUS "pytest available: SAT Python tests will run.")
else()
    if(PYTHON_ENFORCE_TESTS)
        message(FATAL_ERROR "pytest is required with PYTHON_ENFORCE_TESTS=ON.")
    else()
        message(WARNING "pytest not found: SAT Python tests disabled.")
    endif()
endif()
if(PYTHON_TESTS)
    add_custom_target(test_python_sat
        COMMAND "${Python3_EXECUTABLE}" -m pytest "${CMAKE_CURRENT_SOURCE_DIR}/test_sat.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_test(NAME test_python_sat
        COMMAND "${Python3_EXECUTABLE}" -m pytest "${CMAKE_CURRENT_SOURCE_DIR}/test_sat.py"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_property(TEST test_python_sat PROPERTY ENVIRONMENT
        "PYTHONPATH=$<TARGET_FILE_DIR:pyariadne>"
    )
    set_property(TEST test_python_sat PROPERTY LABELS "python")
    add_dependencies(test_python_sat pyariadne)
    add_dependencies(tests pyariadne)
endif()
