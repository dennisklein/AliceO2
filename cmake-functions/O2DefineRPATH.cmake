include_guard()

include(GNUInstallDirs)

#
# o2_define_rpath defines our RPATH settings
#
function(o2_define_rpath)

  if(APPLE)
    set(basePoint @loader_path)
  else()
    set(basePoint $ORIGIN)
  endif()

  # use, i.e. do not skip, the full RPATH in the _build_ tree
  set(CMAKE_SKIP_BUILD_RPATH FALSE PARENT_SCOPE)
  # when building, do not use the install RPATH already (will only be used when
  # actually installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE PARENT_SCOPE)

  # add to the install RPATH the (automatically determined) parts of the RPATH
  # that point to directories outside the build tree
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE PARENT_SCOPE)

  # specify libraries directory relative to binaries one.
  file(RELATIVE_PATH relDir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
       ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

  set(CMAKE_INSTALL_RPATH ${basePoint} ${basePoint}/${relDir} PARENT_SCOPE)

endfunction()
