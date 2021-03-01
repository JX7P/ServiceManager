add_executable(sys.manager Manager.cc ${PROJECT_SOURCE_DIR}/vendor/sqlite/sqlite3.c)
target_include_directories(sys.manager
  PRIVATE
    ${PROEJCT_SOURCE_DIR}/vendor/sqlite)
target_link_libraries(sys.manager eci rt Threads::Threads ${CMAKE_DL_LIBS})
set_property(TARGET sys.manager PROPERTY CXX_STANDARD 11)