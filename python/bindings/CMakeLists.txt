include_directories(${Python3_INCLUDE_DIRS})
get_target_property(PYTHON_COMMON_SOURCE_DIR pyariadne-python-common SOURCE_DIR)

add_library(pyariadne-sat-submodule-obj OBJECT sat_submodule.cpp)
target_include_directories(pyariadne-sat-submodule-obj PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PYTHON_COMMON_SOURCE_DIR}/include"
)
target_link_libraries(pyariadne-sat-submodule-obj PRIVATE
    pyariadne-python-common pybind11::module sat
)
set_target_properties(pyariadne-sat-submodule-obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(pyariadne-sat INTERFACE)
target_sources(pyariadne-sat INTERFACE $<TARGET_OBJECTS:pyariadne-sat-submodule-obj>)
target_include_directories(pyariadne-sat INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<TARGET_PROPERTY:pyariadne-bdd,INTERFACE_INCLUDE_DIRECTORIES>
)
# The combined Python module registers both BDD and CDCL before SAT.
# Include all transitively registered module objects to avoid undefined
# module symbols at import/link time.
target_link_libraries(pyariadne-sat INTERFACE
    pyariadne-bdd
    pyariadne-cdcl
)

ariadne_add_python_module(NAME sat MODULE_SOURCE sat_module.cpp)
