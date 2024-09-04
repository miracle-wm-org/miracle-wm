# Running miracle as a systemd user session

> Most of my understanding about how Wayland compositors interact with systemd
> is taken from here https://github.com/swaywm/sway/wiki/Systemd-integration
> and here https://github.com/alebastr/sway-systemd

Miracle offers a standard way of integrating with `systemd` that is largely
derived from https://github.com/alebastr/sway-systemd.

## Setup
From the root of the project:

```sh
cd session
cp etc/systemd/user/*.target <SYSTEMD_USER_UNIT_DIRECTORY> # most likely /usr/lib/systemd/user/, $XDG_CONFIG_HOME/systemd/user/ or ~/.config/systemd/user

cp /usr/lib/systemd/user/, $XDG_CONFIG_HOME/systemd/user/ or ~/.config/systemd/user
cp usr/bin/libexec/miracle-wm/session.sh /usr/local/libexec/miracle-wm/session.sh
```

Then, in your `~/.config/miracle-wm.yaml`, add:

```yaml
startup_apps:
  - command: /usr/local/libexec/miracle-wm/session.sh
```

It is important that this is the first application that is executed, as the
following apps will rely on the environment variables set here.

Finally, log out and log back into miracle. Once running, try to run:

```sh
echo $XDG_SESSION_DESKTOP  # This should be "miracle-wm"
```
