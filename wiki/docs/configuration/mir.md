# Mir's Built-in Configuration

> Please note that Mir's built-in configuration **is distinct from the configuration defined in
> the [configuration section](introduction.md)**. The two may indeed overlap with one another. There is a task
> in miracle-wm to [fix this](https://github.com/mattkae/miracle-wm/issues/93).

Mir provides a built-in configuration on top of the one that miracle-wm defines.

## Example

```
# ~/.config/miracle-wm.config

x11-window-title=miracle-wm
idle-timeout=600
display-config=sidebyside
add-wayland-extensions=zwp_idle_inhibit_manager_v1:zwp_input_method_v1
```

## Location

This file can be found at:

```
~/.config/miracle-wm.config
```

or at `$XDG_CONFIG_HOME/miracle-wm.config` if `$XDG_CONFIG_HOME` is defined.

## Format

This configuration is in a typical key-value pair format:

```
key=value
```

The list of valid `key`s can be found by running:

```sh
miracle-wm --help
```

The keys are the names of the commandline arguments.
