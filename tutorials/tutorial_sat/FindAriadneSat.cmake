# Try to find the standalone Ariadne SAT library.
#
# Once done this will define:
#
#  AriadneSat_FOUND - system has Ariadne SAT
#  ARIADNE_SAT_INCLUDE_DIRS - the Ariadne SAT include directories
#  ARIADNE_SAT_LIBRARIES - libraries required to use Ariadne SAT
#
# and, when found:
#
#  AriadneSat::ariadne-sat - imported library target
#
# Copyright  2026  Luca Geretti

find_library(ARIADNE_SAT_LIBRARY NAMES ariadne-sat)
find_path(ARIADNE_SAT_INCLUDE_DIR ariadne-sat.hpp PATH_SUFFIXES ariadne-sat)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AriadneSat DEFAULT_MSG
  ARIADNE_SAT_LIBRARY
  ARIADNE_SAT_INCLUDE_DIR
)

set(ARIADNE_SAT_FOUND ${AriadneSat_FOUND})

if(AriadneSat_FOUND)
  set(ARIADNE_SAT_INCLUDE_DIRS ${ARIADNE_SAT_INCLUDE_DIR})
  set(ARIADNE_SAT_LIBRARIES ${ARIADNE_SAT_LIBRARY})

  if(NOT TARGET AriadneSat::ariadne-sat)
    add_library(AriadneSat::ariadne-sat UNKNOWN IMPORTED)
    set_target_properties(AriadneSat::ariadne-sat PROPERTIES
      IMPORTED_LOCATION "${ARIADNE_SAT_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ARIADNE_SAT_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(
  ARIADNE_SAT_INCLUDE_DIR
  ARIADNE_SAT_LIBRARY
)
