configure_file (${HDR}/eci/Platform.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/eci/Platform.h)

add_library (eci-core Core.c)
target_include_directories (eci-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${HDR}>)

add_library (eci Event-${ECI_EVENT_DRIVER}.cc Logger.cc WSRPC.cc)
target_link_libraries (eci eci-core ucl)

# installation

set_target_properties(eci PROPERTIES
    PUBLIC_HEADER "${HDR}/eci/ECI.hh;${HDR}/eci/WSRPC.hh;")

install(TARGETS eci-core eci
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eci)

install(TARGETS eci DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eci)