# need:
# GPI_COMPILE_FLAGS
# GPI_INCLUDE_PATH
# GPI_LIBRARIES
include (FindPackageHandleStandardArgs)

if (DEFINED ENV{GPI_2_BASE} )
    set ( GPI_DIR "$ENV{GPI_2_BASE}")
endif()

FIND_PATH(
  GPI_INCLUDE_PATH
    GASPI.h
  HINTS
    ${GPI_DIR}/include
)

FIND_LIBRARY( GPI_LIBRARIES
  NAMES libGPI2.a
  HINTS ${GPI_DIR}/lib64
)

find_package_handle_standard_args (GPI
    DEFAULT_MSG GPI_LIBRARIES GPI_INCLUDE_PATH)
