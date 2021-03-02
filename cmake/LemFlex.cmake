set(FLEX_EXECUTABLE flex)

function(FlexComp path)
GET_FILENAME_COMPONENT(file ${path} NAME_WE)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file}.l.h
        ${CMAKE_CURRENT_BINARY_DIR}/${file}.l.c
    COMMAND ${FLEX_EXECUTABLE}
    ARGS -o${CMAKE_CURRENT_BINARY_DIR}/${file}.l.c
        --header-file=${CMAKE_CURRENT_BINARY_DIR}/${file}.l.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${path}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${path}
)
endfunction(FlexComp)

function(FlexCXXComp path)
GET_FILENAME_COMPONENT(file ${path} NAME_WE)
add_custom_command(
OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file}.l.h
	${CMAKE_CURRENT_BINARY_DIR}/${file}.l.cxx
COMMAND ${FLEX_EXECUTABLE}
ARGS -o${CMAKE_CURRENT_BINARY_DIR}/${file}.l.cxx
    --header-file=${CMAKE_CURRENT_BINARY_DIR}/${file}.l.h
    ${CMAKE_CURRENT_SOURCE_DIR}/${path}
DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${path}
)
endfunction(FlexCXXComp)

function(LemonComp path)
GET_FILENAME_COMPONENT(file ${path} NAME_WE)
add_custom_command(
    OUTPUT ${file}.tab.h ${file}.tab.c ${file}.out
    COMMAND $<TARGET_FILE:lemon>
    ARGS ${path} 
        -d. -p -T${CMAKE_SOURCE_DIR}/vendor/lemon/lempar.tpl
    DEPENDS $<TARGET_FILE:lemon>
    MAIN_DEPENDENCY ${path})
endfunction(LemonComp)

function(LemonCXXComp path)
message("OUT  ${CMAKE_CURRENT_SOURCE_DIR}/${path} ")
GET_FILENAME_COMPONENT(file ${path} NAME_WE)
add_custom_command(
    OUTPUT ${file}.tab.cxx ${file}.tab.h ${file}.out
    COMMAND $<TARGET_FILE:lemonxx>
    ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${path} 
        -d. -p -T${CMAKE_SOURCE_DIR}/vendor/lemon/lemxx.tpl
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${path})
endfunction(LemonCXXComp)