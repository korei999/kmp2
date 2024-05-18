![kmp9](https://github.com/korei999/kmp2/assets/93387739/c9f11af2-64b5-40d5-93b3-b0b1bd2bc4f9)

Formats supported: flac, opus, mp3, ogg, wav, caf, aif.
### Usage:
- `kmp *`, `kmp **/* 2> /dev/null` or whatever your shell can do.
- Navigate with vim-like keybinds.
- `h` and `l` seek back/forward.
- `o` and `i` next/prev song.
- Searching: `/` or `?`, then `n` and `N` jump to next/prev found string.
- `9` and `0` volume.
- `t` set time like `4:20`, `40` or `60%`.
- `:` jump to selected number in the playlist.
- `z` center around currently playing song.
- `r` repeat after last song.
- `m` mute.
- `q` quit.
- `[` and `]` playback speed shifting fun. `\` Set original speed back.
- `v` toggle visualizer.
- 'ctrl-l' refresh screen.

### Dependencies:
fedora: `sudo dnf install pipewire0.2-devel pipewire-devel ncurses-devel libsndfile-devel meson fmt-devel(optional)`

### Install:
```
meson setup build
ninja -C build
sudo ninja -C build install
```
