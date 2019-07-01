#
# Finds the rapidjson (header-only) library using the CONFIG file provided by
# RapidJSON and add the RapidJSON::RapidJSON imported targets on top of it
#

find_package(RapidJSON CONFIG QUIET)

if(NOT RapidJSON_INCLUDE_DIR)
  set(RapidJSON_FOUND FALSE)
  if(RapidJSON_FIND_REQUIRED)
    message(FATAL_ERROR "RapidJSON not found")
  endif()
else()
  set(RapidJSON_FOUND TRUE)
endif()

mark_as_advanced(RapidJSON_INCLUDE_DIR)

get_filename_component(inc ${RapidJSON_INCLUDE_DIR} ABSOLUTE)

if(RapidJSON_FOUND AND NOT TARGET RapidJSON::RapidJSON)
  add_library(RapidJSON::RapidJSON IMPORTED INTERFACE)
  set_target_properties(RapidJSON::RapidJSON
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${inc})
endif()

unset(inc)
