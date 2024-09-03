find_package(PkgConfig)
pkg_check_modules(JSONC json-c REQUIRED)

add_executable(miraclemsg
    ipc.h
    ipc_client.cpp ipc_client.h
    main.cpp)

target_include_directories(miraclemsg PUBLIC SYSTEM
    ${JSONC_INCLUDE_DIRS})

target_link_libraries(miraclemsg
    ${JSONC_LDFLAGS})
                                   
install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/miraclemsg
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
