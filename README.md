![kmp9](https://github.com/korei999/kmp2/assets/93387739/c9f11af2-64b5-40d5-93b3-b0b1bd2bc4f9)

### Features:
- Formats: flac, opus, mp3, ogg, wav, caf, aif.
- MPRIS D-Bus controls.
- Any playback speed (no pitch correction).
- Visualizer.

### Usage:
- Play each song in the directory: `kmp *`, or recursively: `kmp **/*`.
- With no arguments, stdin with pipe can be used: `find /path -iname '*.mp3' | kmp` or whatever your shell can do.
- Navigate with vim-like keybinds.
- `h` / `l` seek back/forward.
- `o` / `i` next/prev song.
- Searching: `/` or `?`, then `n` and `N` jump to next/prev found string.
- `9` / `0` change volume, or `(` / `)` for smaller steps.
- `t` select time: `4:20`, `40` or `60%`.
- `:` jump to selected number in the playlist.
- `z` center around currently playing song.
- `r` / `R` cycle between repeat methods (None, Track, Playlist).
- `m` mute.
- `q` quit.
- `[` / `]` playback speed shifting fun. `\` Set original speed back.
- `v` toggle visualizer.
- `ctrl-l` refresh screen.

### Dependencies:
fedora: `sudo dnf install cmake pipewire0.2-devel pipewire-devel ncurses-devel libsndfile-devel # (optional) systemd-devel fmt-devel`\
ubuntu: `sudo apt install cmake libpipewire0.3-dev libsndfile1-dev libncurses-dev # (optional) libsystemd-dev libfmt-dev`

### Install:
```
cmake -S . -B build/
cmake --build build/ -j
sudo cmake --install build/
```

### Uninstall
```
sudo xargs rm < ./build/install_manifest.txt
```
