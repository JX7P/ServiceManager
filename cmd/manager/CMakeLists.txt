MakeHeader(${SHARESRC}/repository_schema.sql
  repository_schema.sql.h)
add_executable(sys.manager Backend.cc Manager.cc
  ${CMAKE_CURRENT_BINARY_DIR}/repository_schema.sql.h
  ${VENDORSRC}/sqlite/sqlite3.c)
target_include_directories(sys.manager
  PRIVATE
    ${PROEJCT_SOURCE_DIR}/vendor/sqlite)
target_link_libraries(sys.manager eci rt Threads::Threads ${CMAKE_DL_LIBS})
set_property(TARGET sys.manager PROPERTY CXX_STANDARD 11)