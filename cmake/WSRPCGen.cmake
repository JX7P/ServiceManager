function(WSRPCGen)
    set(options "")
    set(oneValueArgs INPUT)
    set(oneValueArgs OUTPUT)
    set(multiValueArgs INCLUDE_DIRECTORIES WHATEVER)
    set(wsrpcgen $<TARGET_FILE:wsrpcgen>)
    cmake_parse_arguments(WS "${options}" "${oneValueArgs}"
                      "${multiValueArgs}" ${ARGN} )

    set(file ${OUTPUT})
    get_filename_component(dir ${WS_INPUT} DIRECTORY)
    set(hdr ${CMAKE_CURRENT_BINARY_DIR}/${file}.hh)
    set(svc ${CMAKE_CURRENT_BINARY_DIR}/${file}_svc.cc)
    set(clnt ${CMAKE_CURRENT_BINARY_DIR}/${file}_clnt.cc)
    set(conv ${CMAKE_CURRENT_BINARY_DIR}/${file}_conv.cc)

    list(APPEND incDirFlags)
    foreach(incDir_in ${WS_INCLUDE_DIRECTORIES})
        list(APPEND incDirFlags "-I${incDir_in}")
    endforeach()

    add_custom_command(OUTPUT ${hdr}
        COMMAND rm
        ARGS -f ${hdr}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -h -o ${CMAKE_CURRENT_BINARY_DIR}/${file} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    add_custom_command(OUTPUT ${svc}
        COMMAND rm
        ARGS -f ${svc}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -s -o ${CMAKE_CURRENT_BINARY_DIR}/${file} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    add_custom_command(OUTPUT ${clnt}
        COMMAND rm
        ARGS -f ${clnt}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -c -o ${CMAKE_CURRENT_BINARY_DIR}/${file} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_custom_command(OUTPUT ${conv}
        COMMAND rm
        ARGS -f ${conv}
        COMMAND ${wsrpcgen}
        ARGS ${incDirFlags} -x -o ${CMAKE_CURRENT_BINARY_DIR}/${file} ${WS_INPUT}
        DEPENDS ${WS_INPUT} ${wsrpcgen}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction(WSRPCGen)