# Installation

## Install
=== "Snap"
    ```
    sudo snap install miracle-wm --classic
    ```

=== "Fedora"
    ```
    sudo dnf install miracle-wm
    ```
    
=== "Ubuntu (mantic and noble)"
    ```
    sudo add-apt-repository ppa:matthew-kosarek/miracle-wm
    sudo apt update
    sudo apt install miracle-wm
    ```
    
=== "Arch Linux (AUR)"
    ```
    https://aur.archlinux.org/packages/miracle-wm
    ```

=== "Nightly"
    ```
    sudo snap install miracle-wm --classic --edge
    ```

## Running
### On login

Once installed, you may select the "Miracle" option from your display manager before you login (e.g. `gdm` or `lightdm`). In most environments, this presents itself as a little "settings" button after you select your name.

Note that if you installed the snap, the option may read "Miracle (snap)" to distinguish it from the binary version.

### Hosted

To run the window manager as a window on your current desktop session, simply run:

```sh
WAYLAND_DISPLAY=wayland-98 miracle-wm
```

Note that this is only useful if you want to test-drive the window manager or do some development on it for yourself.
