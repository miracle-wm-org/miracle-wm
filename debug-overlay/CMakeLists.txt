find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GTK4_LAYER_SHELL REQUIRED IMPORTED_TARGET gtk4-layer-shell-0)
pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET json-c)

add_executable(miracle-wm-debug-overlay
    main.cpp)

# Reuse the IPC socket discovery + framing from the miraclemsg client.
target_link_libraries(miracle-wm-debug-overlay
    PRIVATE
    miracle-ipc
    PkgConfig::GTK4
    PkgConfig::GTK4_LAYER_SHELL
    PkgConfig::JSONC)

install(TARGETS miracle-wm-debug-overlay
    DESTINATION ${CMAKE_INSTALL_BINDIR})
