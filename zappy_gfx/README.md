# Zappy graphical client (gfx)

3D graphical monitor for the Zappy protocol (RFC 4242), built with raylib.
This repository contains only the graphical client (`src/`), organized as:

```
src/
├── main.cpp
├── network/        TcpClient: blocking-socket TCP I/O (select()-based polling,
│                   no fcntl O_NONBLOCK — compliant with the subject's ban on
│                   non-blocking sockets)
├── protocol/       Message, Parser, MessageHandler: turns raw bytes into
│                   RFC 4242 commands and applies them to GameState
├── game/           GameState, Tile, Player, Team, Egg, GameEvent: the
│                   renderer-agnostic world model (no raylib dependency)
└── gfx/            Renderer: raylib 3D scene + HUD, camera, particles,
                    selection, minimap, help overlay
```

## Build

Requires raylib 5.x (headers + static/shared lib) and a C++17 compiler.

```
make            # builds ./gfx (equivalent to `make gfx`)
make clean      # remove object files
make fclean     # remove object files and the binary
make re         # fclean + all
```

If raylib isn't installed system-wide, point the compiler at it, e.g.:

```
CPATH=/path/to/raylib/src LIBRARY_PATH=/path/to/raylib/src make
```

## Run

```
./gfx -p <port> [-h <machine>]
```

- `-p port`      port number of the Zappy server (required)
- `-h machine`   server hostname (default: 127.0.0.1)
- `-help`        usage
- `--screenshot FILE`  save a screenshot a few seconds in and quit (debug/CI use)

## Controls

Left click select · Right drag orbit · Middle drag pan · Wheel zoom ·
WASD/arrows move camera (Shift = faster) · Q/E rotate · R reset camera ·
Tab / Shift+Tab cycle players · F follow selected player ·
+/- change server time unit (sst) · M minimap · L labels · F11 fullscreen ·
Esc clear selection · C reconnect when disconnected

Press H in the app for the same reference.

## Correction-sheet notes

### Graphic client (main checklist)
- **Connects and displays the map**: yes, handles WELCOME/BIENVENUE, GRAPHIC,
  msz/bct/mct.
- **Players, stones, food visible**: yes, distinct shape+color per resource,
  animated players with team colors and level indicators.
- **Click a square for details**: yes, selection panel shows per-resource
  counts, players and eggs present on the tile.
- **Distinguish quantities of a resource**: yes, numeric label on hover/select
  plus stacked instances up to 5 drawn on the tile itself.
- **Click a player for characteristics**: yes, panel shows team, level, tile,
  orientation, inventory.
- **Scroll through the map**: yes, free-fly camera (pan/orbit/zoom) plus a
  clickable minimap.
- **Lock on and follow a player**: yes (`F` key).
- **Sound for broadcasts ("even more epic client")**: yes — see Audio below.

### Bonus part
- **3D client**: yes — raylib 3D scene, not a 2D icon grid.
- **Live server administration**: out of scope here, this is server-side work.
- **Additional bonuses**: this client implements, as distinguishable,
  100%-functional features:
  1. sound on broadcast, with distance-based volume relative to the camera
  2. sound on player join / death / incantation success / incantation failure
  3. a soft ambient background music loop while exploring
  4. an original victory fanfare on game-over (not based on any existing
     composition — written for this project)

### Audio implementation notes
All sounds are synthesized at startup (`Renderer::loadSounds`, sine tones
with a half-sine envelope) rather than loaded from `.wav`/`.ogg` files, so
there is nothing to copy into the Makefile and nothing that can go missing
on a different machine — it "works out of the box on the dumps" per the
subject's requirement. If no audio device is available (e.g. a headless CI
box), `InitAudioDevice`/`IsAudioDeviceReady` fail gracefully and every sound
call is skipped; the client keeps running normally with silence instead of
crashing. Press `N` in the app to mute/unmute the background music; the top
bar shows the current state when an audio device is active.

This package only contains the graphical client; `server` and the AI `client`
binaries referenced elsewhere in the subject are separate deliverables not
included here.
