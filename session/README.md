# Running miracle as a systemd user session

> Most of my understanding about how Wayland compositors interact with systemd
> is taken from here https://github.com/swaywm/sway/wiki/Systemd-integration
> and here https://github.com/alebastr/sway-systemd

Miracle offers a standard way of integrating with `systemd` that is largely
derived from https://github.com/alebastr/sway-systemd.


## Installation
To install miracle with systemd support, simply provide the following option to cmake:

```
cmake -DSYSTEMD_INTEGRATION=1 ..
```

## View Logs
```sh
journalctl --user --identifier miracle-wm
```

## Manual Setup
From the root of the project:

```sh
cd session
cp usr/lib/systemd/user/*.target /usr/lib/systemd/user/   # or $XDG_CONFIG_HOME/systemd/user/ or ~/.config/systemd/user
cp usr/bin/libexec/miracle-wm-session-setup /usr/local/libexec/miracle-wm/miracle-wm-session-setup
```

Then, in your `~/.config/miracle-wm.yaml`, add:

```yaml
startup_apps:
  - command: /usr/local/libexec/miracle-wm/miracle-wm-session-setup
```

It is important that this is the first application that is executed, as the
following apps will rely on the environment variables set here.

Finally, log out and log back into miracle. Once running, try to run:

```sh
echo $XDG_SESSION_DESKTOP  # This should be "miracle-wm"
```
