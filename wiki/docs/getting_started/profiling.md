# Profiling

miracle-wm is a compositor, so "it feels slow" almost always means one of two
things: the render/composite path is missing its frame deadline, or a window
management operation is doing too much work on the event thread. A sampling
profiler answers both, and `tools/miracle-profile.sh` wraps the whole pipeline —
build, record, and render an interactive flamegraph.

Because the interesting call stacks run through Mir (miracle-wm is a `miral`
window management policy, so Mir calls *into* us), the setup also installs Mir's
debug symbols. Without them roughly half of every stack is unreadable.

## One-time setup

```sh
tools/miracle-profile.sh setup
```

This needs `sudo` and will:

1. Enable the `main/debug` component of the Mir PPA and Ubuntu's `ddebs` archive.
2. Install the `-dbgsym` package for each installed Mir runtime library.
3. Lower `kernel.perf_event_paranoid` to `1` so `perf` can profile your own
   processes without running the compositor as root.
4. Fetch [FlameGraph](https://github.com/brendangregg/FlameGraph) into
   `~/.cache/miracle-wm-profile`.

Step 3 is deliberately **not** persistent — it lowers a security boundary, so it
resets on reboot. The command prints how to make it permanent if you want that.

## Build

```sh
tools/miracle-profile.sh build
```

This configures `cmake-build-profile/` with `-DCMAKE_BUILD_TYPE=RelWithDebInfo
-DENABLE_PROFILING=ON`. The `ENABLE_PROFILING` option adds `-g3`, frame pointers
(including leaf frames), and `-rdynamic`, and refuses to coexist with
`ENABLE_LTO` — LTO inlines across translation units and rewrites the very call
graph you are trying to read.

`RelWithDebInfo` rather than `Debug` matters: a `Debug` build is unoptimised, so
its profile is dominated by functions that would have been inlined away in a real
build. It also avoids the `-Werror -Wconversion` wall that the `Debug` branch
turns on.

## Record

There are two ways to capture a profile.

### Attach to a running session (recommended)

This is the one to use for "let me use the compositor and find out what's slow".
Start your session with the profiling binary, then from a terminal *inside* that
session:

```sh
tools/miracle-profile.sh attach -d 30
```

Exercise the compositor during the sampling window — tile and resize windows,
switch workspaces, trigger the overview animation, drag things between outputs.
The profile only contains what you actually did.

### Launch under the profiler

```sh
tools/miracle-profile.sh record
```

Starts the profiling build and records until it exits. If a host Wayland or X11
session is present, miracle-wm runs nested on `WAYLAND_DISPLAY=wayland-98`;
otherwise it takes over the seat's DRM device. Nested is convenient, but note
that it profiles the nested rendering path, not real KMS scanout — for
rendering work, profile on a TTY.

Both forms attach to the compositor **by pid**, which covers all of its threads
but excludes the applications you launch inside it. Otherwise a single browser
window would swamp the graph.

## Read the output

Each run writes to `profiles/<timestamp>-<mode>/`:

| File | What it is |
| --- | --- |
| `flamegraph.svg` | Interactive flamegraph — click to zoom, `Ctrl-F` to search |
| `flamegraph-inverted.svg` | Aggregated by leaf: which single function burns the most CPU, regardless of how it was reached |
| `report.txt` | `perf report` text output, sorted by overhead |
| `perf.data` | Raw samples, for `perf report`/`perf annotate` or Hotspot |
| `meta` | Event, frequency, unwind mode and git revision of the run |

```sh
xdg-open profiles/latest/flamegraph.svg
```

Read the flamegraph by **width, not height** — width is time spent, height is
just stack depth. Start from the widest plateau near the top: that is code that
was on-CPU without calling anything else.

Re-render an existing recording (for example after installing missing symbols)
without re-recording:

```sh
tools/miracle-profile.sh report                    # most recent
tools/miracle-profile.sh report profiles/20260917-113000-attach-4242
```

## Troubleshooting

**Stacks are only one or two frames deep, or full of `[unknown]`.** The script
warns when more than 30% of frames are unresolved. Either the Mir `-dbgsym`
packages are missing (re-run `setup`), or something in the stack was built
without frame pointers. Fall back to DWARF unwinding:

```sh
tools/miracle-profile.sh attach -d 30 --dwarf
```

DWARF unwinding copies a slice of the stack for every sample, so `perf.data` gets
much larger and the overhead is noticeably higher — but it does not depend on
frame pointers at all.

**`Failure to open any events`.** `kernel.perf_event_paranoid` is back above `1`
after a reboot. Re-run `setup`, or set it directly.

**`cycles` does not work.** The hardware PMU is unavailable, typically inside a
VM. The default event is `cpu-clock`, a software timer, which always works. On
bare metal `--event cycles` gives truer CPU attribution.

## Profiling into Mir itself

`setup` installs debug *symbols*, which is enough to see Mir function names in
the flamegraph. If you need to change Mir — or want inlining information the
stripped release build cannot give you — build it from source instead:

```sh
git clone https://github.com/canonical/mir.git && cd mir
sudo apt build-dep ./
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer"
cmake --build build
```

Then point miracle-wm's build and runtime at it:

```sh
export PKG_CONFIG_PATH=/path/to/mir/build:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/path/to/mir/build/lib:$LD_LIBRARY_PATH
tools/miracle-profile.sh build
```

Mir's ABI is unstable between releases, so a source build must be reasonably
close to the version in `debian/control` or miracle-wm will not compile against
it.

## Other viewers

`perf.data` is a standard recording, so the usual tools work:

- **[Hotspot](https://github.com/KDAB/hotspot)** (`sudo apt install hotspot`) —
  a GUI with a timeline, per-thread tracks and caller/callee navigation. Much
  better than a static SVG for finding *when* a stall happened.
  `hotspot profiles/latest/perf.data`
- **[Firefox Profiler](https://profiler.firefox.com)** via
  [samply](https://github.com/mstange/samply) — `samply import profiles/latest/perf.data`
- **[speedscope](https://www.speedscope.app)** — loads `profiles/latest/perf.script`
  directly.
