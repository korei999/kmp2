![kmp7](https://github.com/korei999/kmp2/assets/93387739/46751511-faa0-4280-9e74-c6650e7723ad)

Formats supported: flac, opus, mp3, ogg, wav, caf, aif.
### Usage:
- `kmp *`, `kmp **`, `kmp $(find . | rg "\.mp3|\.wav")` or whatever your shell can do.
- Navigate with vim-like keybinds.
- `h` and `l` seek back/forward.
- Searching: `/` or `?`, then `n` and `N` jump to next/prev found string.
- `9` and `0` volume.
- `t` set time like `4:20`, `40` or `60%`.
- `:` jump to selected number in the playlist.
- `z` center around currently playing song.
- `r` repeat after last song.
- `m` mute.
- `q` quit.

### Dependencies:
fedora: `sudo dnf install pipewire0.2-devel pipewire-devel ncurses-devel libsndfile-devel meson`

### Install:
```
meson setup build
ninja -C build
sudo ninja -C build install
```
