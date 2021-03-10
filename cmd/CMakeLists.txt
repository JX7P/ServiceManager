add_subdirectory(wsrpcgen)
add_subdirectory(manager)

MakeHeader(${SHARESRC}/serviceBundleSchema.json serviceBundleSchema.json.h)
add_executable(addsys addsys.cc
  ${CMAKE_CURRENT_BINARY_DIR}/serviceBundleSchema.json.h)
target_link_libraries(addsys eci sysSqlite3)
target_include_directories(addsys PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_executable(lssys lssys.cc)
target_link_libraries(lssys eci)