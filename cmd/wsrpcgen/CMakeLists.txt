FlexCXXComp(wsrpcgen.l)
LemonCXXComp(wsrpcgen.y)

add_executable(wsrpcgen wsrpcgen.tab.cxx wsrpcgen.l.cxx wsrpcgen.cc gen.cc scope.cc)
target_link_libraries(wsrpcgen eci)
target_include_directories(wsrpcgen PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/vendor/lemon>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)