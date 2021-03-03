MakeHeader(${SHARESRC}/repositorySchema.sql repositorySchema.sql.h)
MakeHeader(${SHARESRC}/volatileRepositorySchema.sql
  volatileRepositorySchema.sql.h)
add_executable(sys.manager Backend.cc Manager.cc
  ${CMAKE_CURRENT_BINARY_DIR}/repositorySchema.sql.h
  ${CMAKE_CURRENT_BINARY_DIR}/volatileRepositorySchema.sql.h
  ${VENDORSRC}/sqlite/sqlite3.c)
target_include_directories(sys.manager
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${VENDORSRC}/sqlite)
target_link_libraries(sys.manager eci rt Threads::Threads ${CMAKE_DL_LIBS})
set_property(TARGET sys.manager PROPERTY CXX_STANDARD 11)