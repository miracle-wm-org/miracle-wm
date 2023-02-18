# Notes
The following file will serve the purpose of documenting my thought processes and progress as I complete the assignment. I am starting this project on February 18, 2023.

## Step 1: Virtual box
- I installed the mir-builder.ova file, imported it into `virtualbox`, and ran it successfully with the provided login
- I successfully built the mir project that was found in the home directory. I followed the documentation here (https://github.com/MirServer/mir/blob/main/doc/getting_involved_in_mir.md) to run the `miral-shell`.

## Step 2: Setting up the miral app
- First I had to install xorg:
```sh
sudo apt update
sudo apt install xinit
```
- I then ran `startx` and, voila, my X server is running.
- Now I just need xwayland:
```sh
sudo apt install xwayland
```
- Finally, I run the `miral-app` in the bin directory and see that we are running.

## Step 3: Hacking
My task is to build a compositor, so I will first define what is that I need in my compositor. As an MVP, I would like to support:
- Window transparency
- Drop shadows