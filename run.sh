# If there's already a compositor for WAYLAND_DISPLAY let Mir choose another
  if [ -O "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY}" ]
  then
    unset WAYLAND_DISPLAY
  fi
  # miral-shell can launch it's own terminal with Ctrl-Alt-T
  MIR_SERVER_ENABLE_X11=1 MIR_SERVER_SHELL_TERMINAL_EMULATOR=${terminal} exec ${gdb} ${bindir} ./build/bin/compositor $*