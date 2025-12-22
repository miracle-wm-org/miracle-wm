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

# XDG Shell protocol (for internal client)
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)

set(XDG_SHELL_PROTOCOL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/xdg-shell.xml")
set(XDG_SHELL_CLIENT_HEADER "${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h")
set(XDG_SHELL_PRIVATE_CODE "${CMAKE_BINARY_DIR}/xdg-shell-protocol.c")

add_custom_command(
        OUTPUT ${XDG_SHELL_CLIENT_HEADER}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${XDG_SHELL_PROTOCOL_FILE} ${XDG_SHELL_CLIENT_HEADER}
        DEPENDS ${XDG_SHELL_PROTOCOL_FILE}
        VERBATIM
)

add_custom_command(
        OUTPUT ${XDG_SHELL_PRIVATE_CODE}
        COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${XDG_SHELL_PROTOCOL_FILE} ${XDG_SHELL_PRIVATE_CODE}
        DEPENDS ${XDG_SHELL_PROTOCOL_FILE}
        VERBATIM
)

add_library(xdg-shell-protocol STATIC
        ${XDG_SHELL_PRIVATE_CODE}
        ${XDG_SHELL_CLIENT_HEADER}
)

target_include_directories(xdg-shell-protocol PUBLIC ${CMAKE_BINARY_DIR})
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
target_include_directories(xdg-shell-protocol PUBLIC ${WAYLAND_CLIENT_INCLUDE_DIRS})
target_link_libraries(xdg-shell-protocol ${WAYLAND_CLIENT_LIBRARIES})