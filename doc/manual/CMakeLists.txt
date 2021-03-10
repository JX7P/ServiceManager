set (MDOCS service_bundle.5)

foreach(MAN_OUT ${MDOCS})
    get_filename_component (MAN_EXT ${MAN_OUT} LAST_EXT)
    string(SUBSTRING ${MAN_EXT} 1 -1 MAN_NUM)

    set(MAN_HTMLOUT ${CMAKE_CURRENT_BINARY_DIR}/${MAN_OUT}.html)
    list(APPEND MANPAGES_OUT ${MAN_OUT})

    add_custom_command (
        OUTPUT ${MAN_HTMLOUT}
        COMMAND mandoc -T html -O toc,style=style.css,man=%N.%S.html
          ${MAN_OUT} > ${MAN_HTMLOUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${MAN_OUT}
        COMMENT "Generating HTML manual page ${MAN_OUT}.html")

    list(APPEND MANPAGES_HTML ${MAN_HTMLOUT})

    install(FILES ${MAN_OUT}
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man${MAN_NUM})
endforeach()

set (INDEX_SRC index.7)
set (INDEX_HTML "${CMAKE_CURRENT_BINARY_DIR}/index.html")

add_custom_command (
    OUTPUT  ${INDEX_HTML}
    COMMAND mandoc -T html -O toc,style=style.css,man=%N.%S.html
      ${INDEX_SRC} > ${INDEX_HTML}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${INDEX_SRC}
    COMMENT "Generating HTML manual page index.html")

add_custom_target (manpages ALL DEPENDS ${MANPAGES_OUT})
add_custom_target (manpages_html DEPENDS ${MANPAGES_HTML} ${INDEX_HTML})
