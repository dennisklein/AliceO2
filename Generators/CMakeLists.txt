o2_setup(NAME Generators)

add_library(${MODULE_NAME}
  src/GeneratorFromFile.cxx
  src/Pythia6Generator.cxx

  include/${MODULE_NAME}/GeneratorFromFile.h
  include/${MODULE_NAME}/Pythia6Generator.h
)
target_link_libraries(${MODULE_NAME} PUBLIC FairRoot::Gen Pythia6)

if(Pythia8_FOUND)
  target_sources(${MODULE_NAME} PRIVATE
    src/Pythia8Generator.cxx
    include/${MODULE_NAME}/Pythia8Generator.h
  )
  target_link_libraries(${MODULE_NAME} PUBLIC Pythia8)
else()
  message(STATUS "Module '${MODULE_NAME}' requires Pythia8 ... deactivated")
endif()

# set(LINKDEF src/GeneratorsLinkDef.h)
# set(LIBRARY_NAME ${MODULE_NAME})
#
# O2_GENERATE_LIBRARY()
