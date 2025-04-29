set(PROTOCOL "wlr-output-management-unstable-v1")
set(PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${PROTOCOL}.xml")
set(GENERATE_FILE "${CMAKE_SOURCE_DIR}/src/wlr-output-management-unstable-v1_wrapper")

add_custom_command(
        OUTPUT ${GENERATE_FILE}.cpp
        OUTPUT ${GENERATE_FILE}.h
        DEPENDS ${PROTOCOL_FILE}
        COMMAND "sh" "-c" "mir_wayland_generator zwlr_ ${PROTOCOL_FILE} header >${GENERATE_FILE}.h"
        COMMAND "sh" "-c" "mir_wayland_generator zwlr_ ${PROTOCOL_FILE} source >${GENERATE_FILE}.cpp"
)

add_custom_target(wlr-output-management-unstable-v1
        DEPENDS ${GENERATE_FILE}.cpp
        DEPENDS ${GENERATE_FILE}.h
        SOURCES
        ${GENERATE_FILE}.cpp
        ${GENERATE_FILE}.h
)