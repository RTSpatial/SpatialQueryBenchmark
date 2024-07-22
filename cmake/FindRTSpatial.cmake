# The following variables are optionally searched for defaults
#  RTSPATIAL_ROOT_DIR:            Base directory where all RTSPATIAL components are found
#
# The following are set after configuration is done:
#  RTSPATIAL_FOUND
#  RTSPATIAL_INCLUDE_DIRS
#  RTSPATIAL_LIBRARIES
#  RTSPATIAL_LIBRARYRARY_DIRS

include(FindPackageHandleStandardArgs)

set(RTSPATIAL_ROOT_DIR "" CACHE PATH "Folder contains RTSpatial")

# We are testing only a couple of files in the include directories
find_path(RTSPATIAL_INCLUDE_DIR rtspatial/rtspatial.h
        PATHS ${RTSPATIAL_ROOT_DIR})

find_library(RTSPATIAL_LIBRARY rtspatial)

find_package_handle_standard_args(RTSpatial DEFAULT_MSG RTSPATIAL_INCLUDE_DIR RTSPATIAL_LIBRARY)


if (RTSPATIAL_FOUND)
    set(RTSPATIAL_INCLUDE_DIRS ${RTSPATIAL_INCLUDE_DIR})
    set(RTSPATIAL_LIBRARIES ${RTSPATIAL_LIBRARY})
    message(STATUS "Found RTSpatial  (include: ${RTSPATIAL_INCLUDE_DIR}, library: ${RTSPATIAL_LIBRARY})")
    mark_as_advanced(RTSPATIAL_LIBRARY_DEBUG RTSPATIAL_LIBRARY_RELEASE
            RTSPATIAL_LIBRARY RTSPATIAL_INCLUDE_DIR RTSPATIAL_ROOT_DIR)
endif ()