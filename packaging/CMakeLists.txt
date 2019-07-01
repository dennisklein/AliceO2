include(CPack)

install(EXPORT O2Targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/O2
        NAMESPACE O2::
        FILE O2Targets.cmake)

install(FILES O2Config.cmake ../cmake/AddRootDictionary.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/O2)

install(DIRECTORY ../dependencies/ 
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/O2)
