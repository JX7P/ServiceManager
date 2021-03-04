function(WSRPCGen)
    set(wsrpcgen $<TARGET_FILE:wsrpcgen>)
    cmake_parse_arguments(
        WS
        "${options}"
        "INPUT;OUTPUT"
        "$INCLUDE_DIRECTORIES"
        ${ARGN})

    get_filename_component(dir ${WS_INPUT} DIRECTORY)
    set(hdr ${WS_OUTPUT}.hh)
    set(svc ${WS_OUTPUT}_svc.cc)
    set(clnt ${WS_OUTPUT}_clnt.cc)
    set(conv ${WS_OUTPUT}_conv.cc)

    message("OutH: ${hdr}")

    list(APPEND incDirFlags)
    foreach(incDir_in ${WS_INCLUDE_DIRECTORIES})
        list(APPEND incDirFlags "-I${incDir_in}")
    endforeach()

    add_custom_command(OUTPUT ${hdr}
        COMMAND rm
        ARGS -f ${hdr}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -h -o ${WS_OUTPUT} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    add_custom_command(OUTPUT ${svc}
        COMMAND rm
        ARGS -f ${svc}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -s -o ${WS_OUTPUT} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    add_custom_command(OUTPUT ${clnt}
        COMMAND rm
        ARGS -f ${clnt}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -c -o ${WS_OUTPUT} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_custom_command(OUTPUT ${conv}
        COMMAND rm
        ARGS -f ${conv}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -x -o ${WS_OUTPUT} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction(WSRPCGen)