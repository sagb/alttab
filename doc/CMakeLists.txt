find_program(RONN_EXECUTABLE ronn)
set(MAN_PAGE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/alttab.1.ronn)
set(MAN_PAGE ${CMAKE_CURRENT_SOURCE_DIR}/alttab.1)
if(RONN_EXECUTABLE)
    add_custom_command(
        OUTPUT ${MAN_PAGE}
        COMMAND ${RONN_EXECUTABLE} --roff < ${MAN_PAGE_SOURCE} > ${MAN_PAGE}
        COMMAND ${CMAKE_COMMAND} -E echo "Regenerated ${MAN_PAGE} from ${MAN_PAGE_SOURCE}."
        DEPENDS ${MAN_PAGE_SOURCE}
        COMMENT "Regenerating alttab.1 from alttab.1.ronn"
    )
else()
    add_custom_command(
        OUTPUT ${MAN_PAGE}
        COMMAND ${CMAKE_COMMAND} -E touch ${MAN_PAGE}
        COMMAND ${CMAKE_COMMAND} -E echo "Touched ${MAN_PAGE}. Ronn not found."
        DEPENDS ${MAN_PAGE_SOURCE}
        COMMENT "Touching alttab.1 as ronn not found"
    )
endif()
add_custom_target(UpdateManPage ALL
    DEPENDS ${MAN_PAGE}
)
install(FILES ${MAN_PAGE} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

install(FILES wm-setup.md icons.md mobile.md README alttab.ad DESTINATION ${CMAKE_INSTALL_DOCDIR})
