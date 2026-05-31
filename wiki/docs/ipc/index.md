# Introduction
Miracle provides an IPC mechanism for interacting with the compositor at runtime.
This socket is largely in line with both [i3](https://i3wm.org/docs/ipc.html) and 
[sway]((https://github.com/swaywm/sway/blob/master/sway/sway-ipc.7.scd))'s IPC implementation.

 The IPC protocol uses a UNIX socket as the method of communication. The path to the socket
 is stored in the environment variable `MIRACLESOCK`  and, for backwards compatibility with sway, SWAYSOCK,
 and, for backwards compatibility with i3, I3SOCK.

## Format
The format for messages and replies is:

```
    <magic-string> <payload-length> <payload-type> <payload>

Where
    <magic-string> is i3-ipc, for compatibility with i3
    <payload-length> is a 32-bit integer in native byte order
    <payload-type> is a 32-bit integer in native byte order
```

For example, sending the exit command would look like the following hexdump:

```
00000000 | 69 33 2d 69 70 63 04 00 00 00 00 00 00 00 65 78 |i3-ipc........ex|
00000010 | 69 74                                           |it              |
```

The payload for replies will be a valid serialized JSON data structure.

## Messages
Each message is associated with a "Message Type" that is given by the number in parentheses. This is
the value that users will send from the client when they want to send the corresponding message.

- [RUN_COMMAND (0)](run_command.md)
- [GET_WORKSPACES (1)](get_workspaces.md)
- [SUBSCRIBE (2)](subscribe.md)
- [GET_OUTPUTS (3)](get_outputs.md)
- [GET_TREE (4)](get_tree.md)
- [GET_MARKS (5)](get_marks.md)
- [GET_VERSION (7)](get_version.md)
- [GET_BINDING_MODES (8)](get_binding_modes.md)
- [SEND_TICK (10)](send_tick.md)
- [SYNC (11)](sync.md)
- [GET_BINDING_STATE (12)](get_binding_state.md)

For those familiar with sway, miracle will always lack support for particular messages
such as:

- `GET_CONFIG`
- `GET_BAR_CONFIG`

The following are unimplemented, but may be implemented in the future:

- `GET_INPUTS`
- `GET_SEATS`

## Events
- [workspace (0x80000000)](./events/workspace.md)
- [output (0x80000001)](./events/output.md)
- [mode (0x80000002)](./events/mode.md)
- [window (0x80000003)](./events/window.md)
- [binding (0x80000005)](./events/binding.md)
- [shutdown (0x80000006)](./events/shutdown.md)
- [tick (0x80000007)](./events/tick.md)

## Commands
- [exec](./commands/exec.md)
- [split](./commands/split.md)
- [layout](./commands/layout.md)
- [focus](./commands/focus.md)
- [move](./commands/move.md)
- [mark](./commands/mark.md)
- [resize](./commands/resize.md)
- [swap](./commands/swap.md)
- [sticky](./commands/sticky.md)
- [workspace](./commands/workspace.md)
- [rename](./commands/rename.md)
- [gaps](./commands/gaps.md)
- [scratchpad](./commands/scratchpad.md)
- [nop](./commands/nop.md);
