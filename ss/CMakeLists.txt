if(NOT TARGET bdd)
    ensure_submodule_initialized(bdd)
    add_subdirectory(bdd)
endif()
