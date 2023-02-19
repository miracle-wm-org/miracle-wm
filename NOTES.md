# Notes
The following file will serve the purpose of documenting my thought processes and progress as I complete the assignment. I am starting this project on February 18, 2023.

## Step 1: Setting up the miral app
.. TODO

## Step 2: Hacking

### Research
I will begin my searching for documentation on how I made write a Mir-based compositor:
1. https://mir-server.io/docs/developing-a-wayland-compositor-using-mir This seems like a good match.
2. I will also look extensively at the `miral-app` demo to see how it is starting the miral server. The `miral-shell` looks like the easiest barebone candidate for me to follow.

### Programming
1. To get started at a reasonabel pace, I copy and pasted the example `FloatingWindowManager` class. I plan to rework it for a `TilingWindowManager` with controls that mimic those if `i3.`
2. Zones seem like a good candidate for the purposes of grouping windows together here.
3. I have successfully created the `WindowGroup` which allows me to continually create new windows horizontally. Now I will hook up keybinds to support vertical.

My task is to build a compositor, so I will first define what is that I need in my compositor. As an MVP, I would like to support:
- Window transparency
- Drop shadows

