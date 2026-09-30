# asciiDJ

A two-deck DJ mixer that runs in the terminal: two decks with speed, scratch and pause, a
3-band EQ and volume per deck, a crossfader, and a file browser, all drawn in ASCII.

## Requirements

- Linux (keyboard input is read through evdev)
- CMake 3.20+
- A C++23 compiler (GCC 14+ or Clang 19+)

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

Keyboard input is read from `/dev/input`, which needs your user in the `input` group
(one-time setup, log out and back in afterwards):

```sh
sudo usermod -aG input $USER
```

Then run it from a directory containing your music (`.wav`, `.mp3`, `.flac`):

```sh
cd ~/Music
/path/to/asciiDJ/build/asciiDJ
```

Quit with `Ctrl+C`.

## Controls

Keys are bound by physical position (QWERTY layout shown). Bindings live in
`include/InputSchema.hpp`, speeds and ranges in `include/InputTuning.hpp`.

Hold `Left Shift` while touching any control to reset it to its default.

| | Left deck | Right deck |
|---|---|---|
| Speed up / down | `w` / `q` | `p` / `o` |
| Spin forward / back | `s` / `a` | `;` / `l` |
| Pause / play | `x` | `.` |
| Volume up / down | `t` / `g` | `y` / `h` |
| Highs up / down | `r` / `e` | `i` / `u` |
| Mids up / down | `f` / `d` | `k` / `j` |
| Lows up / down | `v` / `c` | `,` / `m` |

| Crossfader | |
|---|---|
| Left / right | `b` / `n` |

| File browser | |
|---|---|
| Toggle file-select mode | `` ` `` |
| Move selection | `↑` / `↓` |
| Load onto left / right deck | `←` / `→` (in file-select mode) |
