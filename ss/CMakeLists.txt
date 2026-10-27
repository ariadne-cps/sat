if(NOT TARGET bdd)
    ensure_submodule_initialized(bdd)
    add_subdirectory(bdd)
endif()

if(NOT TARGET cdcl)
    ensure_submodule_initialized(cdcl)
    add_subdirectory(cdcl)
endif()
