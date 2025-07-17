find_package(PkgConfig)
pkg_check_modules(JSONC json-c REQUIRED)

# Create a static library for IpcClient and Ipc
add_library(miracle-ipc STATIC
    ipc.h
    ipc_client.cpp ipc_client.h)

target_include_directories(miracle-ipc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}  # So headers like "ipc.h" work from consumers
    ${JSONC_INCLUDE_DIRS})

target_link_libraries(miracle-ipc
    ${JSONC_LDFLAGS})

# Now define the executable and link it to the library
add_executable(miraclemsg
    main.cpp)

target_link_libraries(miraclemsg
    miracle-ipc)

install(TARGETS miraclemsg
    DESTINATION ${CMAKE_INSTALL_BINDIR})
