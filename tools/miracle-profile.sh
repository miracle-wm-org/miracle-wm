#!/usr/bin/env bash
#
# Sampling-profiler driver for miracle-wm.
#
# Records a `perf` profile of the compositor -- including frames inside the Mir
# libraries, provided the matching *-dbgsym packages are installed -- and renders
# it as an interactive flamegraph SVG.
#
# Run `tools/miracle-profile.sh help` for usage.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${MIRACLE_PROFILE_BUILD_DIR:-$REPO_ROOT/cmake-build-profile}"
BINARY="$BUILD_DIR/miracle-wm"
OUT_ROOT="${MIRACLE_PROFILE_OUT_DIR:-$REPO_ROOT/profiles}"
CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/miracle-wm-profile"
FLAMEGRAPH_DIR="$CACHE_DIR/FlameGraph"

# Mir runtime packages whose debug symbols we need to see into the compositor's
# own call stacks. The dbgsym package names carry the ABI number, so derive them
# from what is actually installed rather than hardcoding a version.
MIR_RUNTIME_PKG_GLOBS=(
    'libmiral[0-9]*' 'libmirserver[0-9]*' 'libmircommon[0-9]*'
    'libmirplatform[0-9]*' 'libmirwayland[0-9]*' 'libmircore[0-9]*'
    'mir-platform-graphics-*' 'mir-platform-rendering-*' 'mir-platform-input-*'
)

die()  { printf '\033[31merror:\033[0m %s\n' "$*" >&2; exit 1; }
warn() { printf '\033[33mwarning:\033[0m %s\n' "$*" >&2; }
info() { printf '\033[36m==>\033[0m %s\n' "$*" >&2; }

# --------------------------------------------------------------------------
# Preflight
# --------------------------------------------------------------------------

require_perf() {
    command -v perf >/dev/null || die "perf not found. Install it with: sudo apt install linux-tools-generic"

    local paranoid
    paranoid=$(cat /proc/sys/kernel/perf_event_paranoid)
    if (( paranoid > 1 )); then
        die "kernel.perf_event_paranoid is $paranoid, which blocks unprivileged profiling.
       Run 'tools/miracle-profile.sh setup' first, or manually:
           sudo sysctl -w kernel.perf_event_paranoid=1 kernel.kptr_restrict=0"
    fi
}

require_flamegraph() {
    if [[ ! -x "$FLAMEGRAPH_DIR/flamegraph.pl" ]]; then
        info "Fetching FlameGraph into $FLAMEGRAPH_DIR"
        mkdir -p "$CACHE_DIR"
        git clone -q --depth 1 https://github.com/brendangregg/FlameGraph.git "$FLAMEGRAPH_DIR" \
            || die "could not fetch FlameGraph (needs network access)"
    fi
}

require_build() {
    [[ -x "$BINARY" ]] || die "no profiling build at $BINARY -- run: tools/miracle-profile.sh build"

    # A binary without frame pointers silently produces one-frame-deep flamegraphs,
    # which looks like a working profile but tells you nothing. Catch it up front.
    if ! grep -q 'ENABLE_PROFILING:BOOL=ON' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
        warn "$BUILD_DIR was not configured with -DENABLE_PROFILING=ON; stacks may be truncated"
    fi
}

# Warn once if Mir has no debug symbols, since that is the single biggest
# difference between a useful and a useless flamegraph for this project.
check_mir_symbols() {
    local miral
    miral=$(ldconfig -p 2>/dev/null | awk '/libmiral\.so\./ {print $NF; exit}')
    [[ -n "$miral" ]] || return 0

    local build_id
    build_id=$(readelf -n "$miral" 2>/dev/null | awk '/Build ID:/ {print $3}')
    [[ -n "$build_id" ]] || return 0

    local dbg="/usr/lib/debug/.build-id/${build_id:0:2}/${build_id:2}.debug"
    if [[ ! -f "$dbg" ]]; then
        warn "Mir debug symbols are not installed -- frames inside libmiral/libmirserver
         will show as raw addresses. Fix with: tools/miracle-profile.sh setup"
    fi
}

# --------------------------------------------------------------------------
# Commands
# --------------------------------------------------------------------------

cmd_setup() {
    local codename; codename=$(. /etc/os-release && echo "$VERSION_CODENAME")

    # The Mir PPA is added under a release-specific filename, and which PPA is in
    # use depends on whether you track release/rc/dev -- so find it rather than
    # guessing the name.
    local ppa_sources=()
    mapfile -t ppa_sources < <(ls /etc/apt/sources.list.d/mir-team-ubuntu-*.sources 2>/dev/null || true)

    cat <<EOF
This needs root. It will:

  1. Enable the debug-symbol component of the Mir PPA and the Ubuntu ddebs archive
  2. Install *-dbgsym for the installed Mir runtime libraries
  3. Relax kernel.perf_event_paranoid so perf can profile your own processes

Step 3 is NOT persistent across reboots (by design -- it lowers a security
boundary). Re-run this command after a reboot, or see the note at the end.

EOF
    read -r -p "Proceed? [y/N] " reply
    [[ "$reply" =~ ^[Yy]$ ]] || { info "Aborted."; return 0; }

    # 1. Debug symbol sources. The Mir PPA publishes dbgsym into a 'main/debug'
    #    component of the same archive; Ubuntu's own live on ddebs.ubuntu.com.
    if (( ${#ppa_sources[@]} == 0 )); then
        warn "no mir-team PPA found in /etc/apt/sources.list.d -- Mir debug symbols
         may be unavailable. Add one with: sudo add-apt-repository ppa:mir-team/release"
    fi
    local src
    for src in "${ppa_sources[@]}"; do
        if ! grep -q 'main/debug' "$src"; then
            info "Enabling main/debug component on $(basename "$src")"
            sudo sed -i 's|^Components: main$|Components: main main/debug|' "$src"
        fi
    done
    if [[ ! -f /etc/apt/sources.list.d/ddebs.sources ]]; then
        info "Enabling ddebs.ubuntu.com"
        sudo tee /etc/apt/sources.list.d/ddebs.sources >/dev/null <<EOF
Types: deb
URIs: http://ddebs.ubuntu.com
Suites: $codename $codename-updates $codename-proposed
Components: main restricted universe multiverse
Signed-By: /usr/share/keyrings/ubuntu-dbgsym-keyring.gpg
EOF
        sudo apt-get install -y ubuntu-dbgsym-keyring || \
            warn "could not install ubuntu-dbgsym-keyring; Ubuntu ddebs may fail to verify"
    fi
    sudo apt-get update

    # 2. Resolve installed Mir runtime packages -> their dbgsym counterparts.
    info "Resolving Mir debug symbol packages"
    local installed=() wanted=() pkg
    mapfile -t installed < <(
        dpkg-query -W -f='${binary:Package} ${db:Status-Status}\n' "${MIR_RUNTIME_PKG_GLOBS[@]}" 2>/dev/null \
            | awk '$2 == "installed" {sub(/:.*/, "", $1); print $1}' | sort -u
    )
    for pkg in "${installed[@]}"; do
        if apt-cache show "${pkg}-dbgsym" >/dev/null 2>&1; then
            wanted+=("${pkg}-dbgsym")
        else
            warn "no dbgsym published for $pkg"
        fi
    done

    if (( ${#wanted[@]} )); then
        info "Installing: ${wanted[*]}"
        sudo apt-get install -y "${wanted[@]}"
    else
        warn "found no Mir dbgsym packages to install"
    fi

    # 3. perf permissions.
    info "Relaxing perf restrictions for this boot"
    sudo sysctl -w kernel.perf_event_paranoid=1 kernel.kptr_restrict=0

    require_flamegraph

    cat <<'EOF'

Setup complete.

To make the perf settings persist across reboots (optional, lowers a security
boundary permanently):

    echo -e 'kernel.perf_event_paranoid=1\nkernel.kptr_restrict=0' \
        | sudo tee /etc/sysctl.d/99-miracle-profiling.conf

Next:  tools/miracle-profile.sh build
EOF
}

cmd_build() {
    info "Configuring $BUILD_DIR (RelWithDebInfo + ENABLE_PROFILING)"
    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DENABLE_PROFILING=ON \
        -DENABLE_LTO=OFF \
        -DENABLE_TESTS=OFF \
        "$@"
    cmake --build "$BUILD_DIR" -j"$(nproc)"
    info "Built $BINARY"
}

# Shared recording core. Records $TARGET_PID into $1 until $2 (a shell function or
# command) returns, then stops perf cleanly.
#
# perf runs in the background and is stopped with SIGINT rather than being given the
# workload as `perf record -- cmd`: that form makes perf sample the command and all
# its children, which here would mean every terminal and application launched inside
# the compositor session. Attaching by pid keeps all of the compositor's threads
# (they share its tgid) and nothing else.
do_record() {
    local outdir="$1"; shift

    mkdir -p "$outdir"

    info "Recording pid $TARGET_PID ($EVENT @ ${FREQ}Hz, --call-graph $CALLGRAPH)"
    perf record \
        --output "$outdir/perf.data" \
        --freq "$FREQ" \
        --call-graph "$CALLGRAPH" \
        --event "$EVENT" \
        --pid "$TARGET_PID" &
    local perf_pid=$!

    # If perf could not open its events it dies immediately; surface that rather
    # than running the whole workload and reporting "no samples" at the end.
    sleep 0.5
    kill -0 "$perf_pid" 2>/dev/null || die "perf failed to start recording (see error above)"

    # Run the workload window.
    "$@" || true

    # SIGINT is how perf is meant to be stopped: it flushes its buffers and writes
    # the header. SIGTERM/SIGKILL leave an unreadable perf.data.
    kill -INT "$perf_pid" 2>/dev/null || true
    wait "$perf_pid" 2>/dev/null || true

    [[ -s "$outdir/perf.data" ]] || die "perf produced no samples"

    # Bake the symbol mapping into the recording so the report is reproducible even
    # if the build tree or the Mir packages change afterwards.
    perf buildid-cache --add "$BINARY" >/dev/null 2>&1 || true

    # Persist how this was recorded, so `report` re-renders with the real parameters
    # rather than whatever the defaults happen to be at re-render time.
    {
        echo "EVENT=$EVENT"
        echo "FREQ=$FREQ"
        echo "CALLGRAPH=$CALLGRAPH"
        echo "TARGET_PID=$TARGET_PID"
        echo "REVISION=$(git -C "$REPO_ROOT" describe --tags --always --dirty 2>/dev/null || echo unknown)"
    } > "$outdir/meta"

    ln -sfn "$outdir" "$OUT_ROOT/latest"
    info "Raw profile: $outdir/perf.data"
}

# Workload window: block until the profiled process exits.
wait_for_exit() {
    while kill -0 "$TARGET_PID" 2>/dev/null; do sleep 0.5; done
}

# Workload window: fixed duration.
wait_for_duration() {
    sleep "$DURATION"
}

cmd_record() {
    require_perf; require_build; check_mir_symbols

    local outdir="$OUT_ROOT/$(date +%Y%m%d-%H%M%S)-record"
    TARGET_PID=""

    # Nested if there is a host session to nest into, otherwise this takes over the
    # TTY/DRM device -- which is the realistic case for profiling real rendering.
    if [[ -n "${WAYLAND_DISPLAY:-}" || -n "${DISPLAY:-}" ]]; then
        info "Host session detected; miracle-wm will run nested on WAYLAND_DISPLAY=wayland-98"
        info "Quit the nested compositor to stop recording."
    else
        info "No host session; miracle-wm will take over this seat's DRM device."
        info "Quit the compositor to stop recording."
    fi

    # Start the compositor first and profile it by pid. Recording the launch
    # directly (perf record -- miracle-wm) would inherit into every terminal and
    # application started inside the session and swamp the graph; attaching by pid
    # still covers all of the compositor's own threads, which share its tgid.
    WAYLAND_DISPLAY=wayland-98 "$BINARY" "$@" &
    TARGET_PID=$!
    trap 'kill "$TARGET_PID" 2>/dev/null || true' INT TERM

    # Give it a moment to fail fast on a bad command line rather than recording
    # an empty profile of a process that already exited.
    sleep 1
    kill -0 "$TARGET_PID" 2>/dev/null || die "miracle-wm exited immediately; check its output above"

    do_record "$outdir" wait_for_exit
    render "$outdir"
}

cmd_attach() {
    require_perf; check_mir_symbols

    local pid="${PID:-}"
    if [[ -z "$pid" ]]; then
        mapfile -t candidates < <(pgrep -x miracle-wm || true)
        (( ${#candidates[@]} )) || die "no running miracle-wm found (use --pid to be explicit)"
        (( ${#candidates[@]} == 1 )) || die "multiple miracle-wm processes: ${candidates[*]} -- pick one with --pid"
        pid="${candidates[0]}"
    fi
    kill -0 "$pid" 2>/dev/null || die "pid $pid is not running"

    TARGET_PID="$pid"
    local outdir="$OUT_ROOT/$(date +%Y%m%d-%H%M%S)-attach-$pid"

    info "Attaching to miracle-wm pid $pid for ${DURATION}s -- exercise the compositor now"
    info "(tile windows, switch workspaces, run the overview animation, drag things around)"
    do_record "$outdir" wait_for_duration
    render "$outdir"
}

# Turn a perf.data into an SVG flamegraph plus a text report.
render() {
    local outdir="$1"
    require_flamegraph
    [[ -s "$outdir/perf.data" ]] || die "no perf.data in $outdir"

    # Prefer the parameters the profile was actually recorded with.
    local REVISION=""
    # shellcheck disable=SC1091
    [[ -f "$outdir/meta" ]] && source "$outdir/meta"

    local script_args=(script --input "$outdir/perf.data" --no-inline)
    # Drop samples from processes the compositor spawned -- otherwise every terminal
    # launched inside the session pollutes the graph.
    if [[ -n "${TARGET_PID:-}" ]]; then
        script_args+=(--pid "$TARGET_PID")
    fi

    info "Folding stacks"
    perf "${script_args[@]}" 2>/dev/null > "$outdir/perf.script"
    [[ -s "$outdir/perf.script" ]] || die "perf script produced nothing"

    "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" --all "$outdir/perf.script" > "$outdir/folded.txt"

    info "Rendering flamegraph"
    "$FLAMEGRAPH_DIR/flamegraph.pl" \
        --title "miracle-wm ${REVISION:-unknown}" \
        --subtitle "$(basename "$outdir")  --  $EVENT @ ${FREQ}Hz" \
        --width 1800 --colors java --hash \
        "$outdir/folded.txt" > "$outdir/flamegraph.svg"

    # An inverted ("icicle") graph aggregates by leaf, which is what actually answers
    # "which single function is burning the most CPU" regardless of how it was reached.
    "$FLAMEGRAPH_DIR/flamegraph.pl" \
        --title "miracle-wm (inverted: hottest leaves)" --subtitle "$(basename "$outdir")" \
        --width 1800 --colors java --hash --reverse --inverted \
        "$outdir/folded.txt" > "$outdir/flamegraph-inverted.svg"

    perf report --input "$outdir/perf.data" --stdio --sort overhead,dso,symbol \
        --percent-limit 0.5 > "$outdir/report.txt" 2>/dev/null || true

    local unresolved
    unresolved=$(awk '{n++} /\[unknown\]|0x[0-9a-f]{6,}/ {u++} END {if (n) printf "%.0f", 100*u/n; else print 0}' "$outdir/perf.script")
    if (( unresolved > 30 )); then
        warn "${unresolved}% of sampled frames are unresolved -- Mir dbgsym packages are
         probably missing, or the stacks need --call-graph dwarf. See 'setup'."
    fi

    cat >&2 <<EOF

$(printf '\033[32mDone.\033[0m')

  Flamegraph      $outdir/flamegraph.svg
  Hottest leaves  $outdir/flamegraph-inverted.svg
  Text report     $outdir/report.txt
  Raw samples     $outdir/perf.data

Open in a browser (click a frame to zoom, Ctrl-F to search):

    xdg-open $outdir/flamegraph.svg
EOF
}

cmd_report() {
    local outdir="${1:-$OUT_ROOT/latest}"
    [[ -d "$outdir" ]] || die "no such profile directory: $outdir"
    render "$(readlink -f "$outdir")"
}

cmd_help() {
    cat <<EOF
miracle-wm sampling profiler

  tools/miracle-profile.sh <command> [options]

Commands
  setup               One-time: install Mir debug symbols, relax perf limits,
                      fetch FlameGraph. Needs sudo.
  build [cmake args]  Configure and build the profiling tree in
                      $(basename "$BUILD_DIR")/ (RelWithDebInfo + ENABLE_PROFILING).
  record [-- args]    Launch the profiling build under perf and record until it
                      exits. Extra args are passed to miracle-wm.
  attach              Attach to an already-running miracle-wm for --duration
                      seconds. This is the one to use for "use it and see what's
                      slow" -- run it from a terminal inside your session.
  report [dir]        Re-render a recording (default: the most recent one).

Options
  -d, --duration SEC  Sampling window for 'attach'        (default: $DURATION)
  -F, --freq HZ       Sampling frequency                  (default: $FREQ)
  -p, --pid PID       Explicit pid for 'attach'
      --dwarf         Unwind with DWARF CFI instead of frame pointers. Much
                      slower and larger, but recovers stacks through any library
                      built without frame pointers. Ubuntu builds Mir with frame
                      pointers, so the default is usually fine -- reach for this
                      if stacks look suspiciously shallow.
      --event EV      perf event (default: $EVENT). Try 'cycles' on bare metal for
                      true CPU attribution, or 'cpu-clock' under virtualisation.

Typical session
  tools/miracle-profile.sh setup
  tools/miracle-profile.sh build
  # start miracle-wm however you normally do, using cmake-build-profile/miracle-wm
  tools/miracle-profile.sh attach -d 30 --dwarf
EOF
}

# --------------------------------------------------------------------------

DURATION=20
FREQ=997
EVENT=cpu-clock
CALLGRAPH=fp
PID=""
TARGET_PID=""

[[ $# -gt 0 ]] || { cmd_help; exit 0; }
command="$1"; shift

args=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--duration) DURATION="$2"; shift 2 ;;
        -F|--freq)     FREQ="$2"; shift 2 ;;
        -p|--pid)      PID="$2"; shift 2 ;;
        --event)       EVENT="$2"; shift 2 ;;
        --dwarf)       CALLGRAPH="dwarf,16384"; shift ;;
        --fp)          CALLGRAPH="fp"; shift ;;
        -h|--help)     cmd_help; exit 0 ;;
        --)            shift; args+=("$@"); break ;;
        *)             args+=("$1"); shift ;;
    esac
done

mkdir -p "$OUT_ROOT"

case "$command" in
    setup)  cmd_setup ;;
    build)  cmd_build "${args[@]+"${args[@]}"}" ;;
    record) cmd_record "${args[@]+"${args[@]}"}" ;;
    attach) cmd_attach ;;
    report) cmd_report "${args[@]+"${args[@]}"}" ;;
    help|-h|--help) cmd_help ;;
    *) die "unknown command '$command' (try: help)" ;;
esac
