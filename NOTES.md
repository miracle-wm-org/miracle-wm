# Notes
The following file will serve the purpose of documenting my thought processes and progress as I complete the assignment. I am starting this project on February 18, 2023.

## Step 1: Setting up the miral app
At first, I went the virtual box route, but then I saw how easy it would be to do this locally, so I opted for that instead. On Arch, I did:
```sh
git clone https://aur.archlinux.org/mir.git
cd mir
makepkg -si
```

and voila. It lives!

## Step 2: Hacking

### Research
I will begin my searching for documentation on how I made write a Mir-based compositor:
1. https://mir-server.io/docs/developing-a-wayland-compositor-using-mir This seems like a good match!

###
My task is to build a compositor, so I will first define what is that I need in my compositor. As an MVP, I would like to support:
- Window transparency
- Drop shadows

