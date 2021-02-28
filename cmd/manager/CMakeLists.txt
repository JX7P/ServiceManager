add_executable(sys.manager Manager.cc)
target_link_libraries(sys.manager eci rt)
set_property(TARGET sys.manager PROPERTY CXX_STANDARD 11)