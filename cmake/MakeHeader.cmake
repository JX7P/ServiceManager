# Generate a C header file defining a static string with the contents of a file.
function(MakeHeader SOURCE OUTPUT)
    message("Generate ${OUTPUT}")
    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${CMAKE_COMMAND}
        ARGS -P ${PROJECT_SOURCE_DIR}/cmake/cmdMakeHeader.cmake
            ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT}
        DEPENDS ${SOURCE})
endfunction(MakeHeader)