if(ECI_SD_NOTIFY_TYPE MATCHES "datagram")
# systemd-compatible
add_library (eci-daemon sd-daemon.c)
elseif(ECI_SD_NOTIFY_TYPE MATCHES "stream")
add_library (eci-daemon sd-daemon-stream.c)
else()
message("Insecure SD-Notify interface")
add_library (eci-daemon alt-sd-daemon.c)
endif()

target_include_directories (eci-daemon PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_link_libraries (eci-daemon eci-core)
