add_subdirectory(eci)
add_subdirectory(eci-daemon)

add_library(sysSqlite3 ${VENDORSRC}/sqlite/sqlite3.c)
target_include_directories(sysSqlite3
  PUBLIC 
    ${VENDORSRC}/sqlite)
target_link_libraries(sysSqlite3 Threads::Threads ${CMAKE_DL_LIBS})