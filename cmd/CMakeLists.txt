add_subdirectory(wsrpcgen)
add_subdirectory(manager)

add_executable(lssys lssys.cc)
target_link_libraries(lssys eci)