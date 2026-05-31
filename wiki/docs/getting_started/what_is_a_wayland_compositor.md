# What is a Wayland compositor?
This document is intended for users who are unfamiliar with compositors, Wayland, or Linux in general. It serves as a high-level introduction to the ecosystem in which this project exists.

To understand `miracle-wm`, we will need to develop our understanding of the Linux graphics stack. In doing so, we'll explore everything from the application layer to the kernel so that we can understand the flow of visual graphics to your computer monitor.


## Key Takeways 🔑

- Applications are written with the help pf **GUI Toolkits**, which abstract away common problems (e.g. text rendering, widgets, scale, keyboard & mouse input, etc)
- Applications talk to a **window manager** using the **Wayland** protocol
    - GUI toolkits implement the client side of this protocol
- Wayland protocols are specified in XML files and do not necessarily have support across different systems
- Wayland is the succesor to the **X11** protocol
- Window managers implement the Wayland protocol
- Window managers rely on **DRM/KMS**, **GBM**, and **EGLStreams** (only Nvidia) for graphics buffer and display management
- Window managers rely on **libinput** for processing input and routing it to the proper windows
- [Mir](https://github.com/canonical/mir) is the C++ library for writing Wayland compositors that `miracle-wm` uses

## How do desktop applications work on Linux?
Imagine that we would like to build a "Todo list" desktop application for computers that are running the Linux operating system. We might want to build our application with the following features:

- A window on the user's desktop
- A button in this window to add a new item
- A modal that popus up when you click the "new item" button
    - This modal might contain a text input for entering the description of the todo item
- A list of added todo items, with each item containing:
    - Text describing the item
    - A checkbox describing the "done" state of the item
    - A button to delete the item
- And so on...

While this application seems simple, we quickly see that there a number of complicated UI widgets and interactions here (e.g. buttons, modals, dynamic lists, text, etc.). On top of that, we might want to have our application translated into a number of different languges, which means that we'll have to render proper fonts for all different types of languages (including left-to-right and right-to-left written languages). Managing all of this is a very difficult problem, especially when you just want to build your todo app.

This is where **GUI tookits** come into play. These toolkits handle the problems of rendering, widgets, animations, localization, mouse input, keyboard input, and much more. When we build our app on top of a toolkit, these common functionalities are given to us for free.

!!! note
    Some of the toolkits on Linux that you may be familiar with are *GTK*, *QT*, *Flutter*, and *Electron*. There are many other toolkits, as well, but these ones are particularly popular.
    
## How do GUI toolkits work?
If you look around the screen that you're reading this webpage on, you probably see a number of other applications in addition to your web browser. You might see a top panel displaying the date and time. You might see a dock displaying icons of the applications that you can open. You might see your favorite text editor like *Visual Studio Code* open. You might even see a todo app! Each of these application wants to render to the screen, receive mouse and keyboard input, and more. 

Linux orchestrates visual applications with the help of a **window manager** (a.k.a a **display server** or **compositor**). This is a single process that starts as soon as you log into your user account. Each application on your desktop tells the window manager that it wants to render *something* to the screen. The window manager is then responsible for combining the images submitted by each application into one final image that is sent off to your screen.

The communication protocol used between applications and the window manager is called **Wayland**. You may think of Wayland in the same way that you think of HTTP. Clients (applications) ask the Server (window manager) to perform some action (e.g. show something on the screen, become fullscreen, etc.). The window manager then has the authority to either accept or deny that request.

Among the responsibilities of the window manager are:

- Placing each application in an appropriate position on the screen(s)
- Stitching together the images displayed by each application and sending the final image off to the screen(s)
- Routing keyboard, mouse, stylus and any other inputs to the proper application (e.g. the currently focused application)
- Maximizing and minimizing applications
- Dragging applications around on the screen
- Switching workspaces
- Alt-tabbing
- Exposing onscreen keyboards
- Handling session locking
- And much, much more


!!! note
    You are probably most familiar with "floating" window managers. These window managers are typical of a baseline "Windows 10" user experience. However, this is not the only way to manage your windows! For example, `miracle-wm` is a tiling window manager, meaning that it treats windows as individual tiles instead of surfaces that are floating in a pool.
    

GUI toolkits abstract the Wayland protocol away from application developers so that they don't have to worry about the details of Waylalnd. Given that the protocol itself is quite complicated, it is a good thing that they do this. You as an application developer _can_ choose to not use a GUI toolkit and implement the client-side of the Wayland protocol yourself, but this would be incredibly time-consuming. On top of that, you would have to implement all of these previously mentioned aspecs of a GUI toolkit (e.g. widgets, input, animations, text rendering etc.). Needless to say, unless you plan to make it your full-time job (or you're a dedicated hobbyist!) it probably isn't worth your time.

## How does wayland work?
The Wayland protocol is specified via XML files. While there exists only a single necessary Wayland protocol (known as [wayland.xml](https://wayland.app/protocols/wayland)), window managers are free to implement as many optional **protocol extensions** as they see fit for their use case. Each protocol provides some new functionality like [screencopy](https://wayland.app/protocols/wlr-screencopy-unstable-v1), [on-screen keyboard support](https://wayland.app/protocols/input-method-unstable-v2), [viewporter for surface cropping and scaling](https://wayland.app/protocols/viewporter), and many more. The list of possible protocols is theoretically as infinite, as window manager developers are free to implement any that they deem appropriate. As an example, KDE defines a number of protocols that are specific to their desktop environment.

While window managers *can* theoretically support any protocols that they like, a protocol will only see use if there are clients for that protocol. As a result, there is a somewhat "official" list of protocols that are maintained by freedesktop.org, which you can find at [https://gitlab.freedesktop.org/wayland/wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols). To get a protocol accepted into this repository, there needs to be widespread consensus among popular Wayland compositors that the protocol is the right solution. Once the protocol gets accepted, each compositor must implement the protocol themselves.

If you'd like to get an overview of Wayland protocols that have been implemented in *some system*, check out [https://wayland.app/protocols/](https://wayland.app/protocols/).

!!! warning
    Wayland protocols are loosely defined. This means that different compositors may implement the same protocol in different ways. Oftentimes, the behavior converges to "whatever GNOME does" or "whatever KDE does" instead of something well-defined in the specification. On top of that, the protocols often fail to define how their protocol should interact with another protocol, which leaves compositor-implementers to fill in the blanks on their own. This is just a warning in case you encounter slightly different behaviors in different environments.

### The history of Wayland
Unfortunately, it doesn't feel right to tell you about Wayland if I don't also provide a quick aside to inform you about its predecessor: **X11**. I won't get into the nitty-gritty details here (especially since I was never an X11 developer) but I will explain enough to clear up some confusion. 

For most of its history, Linux applications used the **X11** (or simply **X**) protocol to handle window management. The X ecosystem is much different compared to the Wayland, and I'm afraid it has led to a bit of confusion among people trying to understand Linux GUI applications for the first time. For starters, there existed only one true implementation of an X11 server on Linux. This was known as the **X.Org Server**. All Linux distributions ran a similar X.Org server underneath the hood. This was known as the "compositor" because it was responsible for "compositing" the final image and sending it off to the displays. The "compositor" was only responsible for compositing however. A separate process usually existed to facilitate the window management aspects of the compositors, which was fittingly called the **window manager**. By "window management aspects", I am refering to features such as:

- Placing new windows
- Allowing windows to be dragged around (or not)
- Moving windows based off keyboard input
- Fullscreening windows
- etc.

Hence, if you ever installed the i3 window manager (as an example), you were installing a *window manager* while the X.Org server remained your *compositor*. This is why you will see the terms "window manager" and "compositor" used interchangeably when descibing a Wayland window manager. In the Wayland world, these are no longer two distinct processes like there were in the X days.

Fast forward to some time in the early 2010s, the X.Org developers finally had enough of X11. The entire ecosystem had a lot of cruft and a lot of hacks. On top of that, X11 failed terribly in terms of security. The X developers figured that it would be much easier to redesign a new protocol from the ground-up. That is when Wayland was born! Unlike X11 whose only real implementation was the X.Org server, Wayland was designed to just be a set of protocol definitions. This means that *anyone* can write their own Wayland server so long as they implement the protocols correctly.

That is as deep as I will go into the history of Wayland. It is obviously much deeper than this and I wasn't around during that time so I can't speak to the specifics. If you ever want to seek out an ex-X11 developer at an XDC conference, I'm sure that they have many horror stories to tell you.

Let's get back to the graphics stack now!


## How do window managers (or "compositors") work?
I will now give a brief introduction into how "window managers" work. However, the details of such a complex piece of software would take a novel to describe accurately, so it should suffice to provide a high-level overview of what a window manager does.

Most importantly, window managers (or "compositors") implement the server side of the Wayland protocol. The window manager can choose to implement any number of Wayland protocols that it likes, but it must at the very least implement the "core" Wayland protocol. Clients connect to the Wayland server via a wayland socket (which you may be able to read by running `echo $WAYLAND_DISPLAY` in you terminal).

This window manager process is *most likely* started by your login manager via `systemd` when you log in, although you may start it manually from a TTY by yoursef. This process has special access to the hardware resources of your system through the `DRM/KMS` subsystem that the kernel exposes. This subsystem exposes an API that allows the window manager to accomplish some of its primary tasks, such as:

- Setting the output dimensions and refresh rates of your monitors
- Managing graphics buffers

Graphics buffers are typically allocated with the help of the **Generic Buffer Management (GBM)** API. This API is part of *Mesa*'s interface. **Mesa** is an open-source implementation of OpenGL. At the time of writing this document, Nvidia provides an **EGLStreams** API for allocating buffers on its GPUs in place of the GBM API. The job of the window manager is to accept the buffers submitted by its various clients, render them all into a buffer per-output, and submit each buffer to the corresponding output.

On top of managing the display, the window manager is also responsible for processing input and routing it to the correct clients. The window manager does this with the help of [libinput](https://wayland.freedesktop.org/libinput/doc/latest/index.html) which is another project run by freedesktop.org. This API monitors input events from the kernel (e.g. mouse clicks, keyboard input, stylus input, etc.) and sends the events off to the window manager so that it can do *something* with them. Most likely the window manager will route these events to a client, but the window manager may also use this input to execute some custom behavior, like a global keyboard shortcut.

While the window managers core responsibility is to show buffers on screen and manage input, it also can potentially do **a lot** more things, many of which are tangentially related to its core responsibility. This might include clipboard management, screen casting, global keybinds, and much more. All of this is up to the window manager developer.

With all of this being said, you might think that developing a Wayland window manager from scratch is a daunting task. And you would be correct to think this! Luckily, other people have thought the same thing, which is why some projects have arisen to handle the commonalities of window manager development for you. Two prominent examples of such libraries are:

- [Mir](https://github.com/canonical/mir): a C++ library for writing Wayland compositors that is maintained by Canonical
- [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots): a C library for writing Wayland compositors
