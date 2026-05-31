# Logs
The default `.desktop` file that is shipped with miracle-wm (most likely at
`/usr/share/wayland-sessions/miracle-wm.desktop`) will run `miracle-wm`
with `systemd-cat`. If you are running miracle from your login manager (such
as `gdm` or `lightdm`), then your logs will most likely be piped to `journald`.
You may access them via:

```sh
journalctl -t miracle-wm
```
