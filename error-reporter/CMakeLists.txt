find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)
pkg_check_modules(GTK4_LAYER_SHELL REQUIRED IMPORTED_TARGET gtk4-layer-shell-0)
pkg_check_modules(JSONC REQUIRED IMPORTED_TARGET json-c)

add_executable(miracle-wm-basic-error-reporter
    main.cpp)

# Reuse the IPC socket discovery + framing from the miraclemsg client.
target_link_libraries(miracle-wm-basic-error-reporter
    PRIVATE
    miracle-ipc
    PkgConfig::GTK4
    PkgConfig::GTK4_LAYER_SHELL
    PkgConfig::JSONC)

# Localization support. The UI chrome is translatable; the error messages
# themselves are displayed verbatim.
target_compile_definitions(miracle-wm-basic-error-reporter PRIVATE
    GETTEXT_PACKAGE="miracle-wm-basic-error-reporter"
    LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

install(TARGETS miracle-wm-basic-error-reporter
    DESTINATION ${CMAKE_INSTALL_BINDIR})
