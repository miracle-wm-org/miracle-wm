#!/bin/sh

for terminal in gnome-terminal.real gnome-terminal weston-terminal qterminal lxterminal x-terminal-emulator xdg-terminal
do
  if which $terminal > /dev/null 2>&1
  then break;
  fi
done

# When testing, we often want to launch a terminal within the Mir session
# while there's an existing desktop: either as a nested window, or after
# VT switching.
#
# This doesn't fit with gnome-terminal's default behaviour: it tries to run as
# a single process and any attempt to launch a second instance:
#
#   1. appears on the desktop that owns the first terminal; and,
#   2. the process exits immediately so we can't wait for the exit
#      (e.g. in miral-app).
if [ "$terminal" != "gnome-terminal.real" ] && [ "$terminal" != "gnome-terminal" ]
then
  # Other terminals default to running as a separate process, which suits us.
  exec $terminal "$@"
else
  # What we do is launch the gnome-terminal-server with a distinct --app-id and,
  # after waiting for it to start, launch gnome-terminal with the same --app-id.
  #
  # In Ubuntu 16.04 and 18.04 gnome-terminal-server is in /usr/lib/gnome-terminal
  # In Fedora and Ubuntu 20.04 gnome-terminal-server is in /usr/libexec/
  for terminal_server in /usr/libexec/gnome-terminal-server /usr/lib/gnome-terminal/gnome-terminal-server /usr/lib/gnome-terminal-server
  do
    if [ -x "$terminal_server" ];  then break; fi
  done

  if ! gdbus introspect --session --dest io.mirserver.Terminal --object-path /io/mirserver/Terminal > /dev/null 2>&1
  then
     $terminal_server --app-id io.mirserver.Terminal&

    if (gdbus help | grep wait > /dev/null)
    then
      gdbus wait       --session io.mirserver.Terminal
    else
      while ! gdbus introspect --session --dest io.mirserver.Terminal --object-path /io/mirserver/Terminal > /dev/null 2>&1
      do
        sleep 0.2
      done
    fi
  fi
  $terminal --app-id io.mirserver.Terminal "$@"
  wait # wait for $terminal_server exit
fi