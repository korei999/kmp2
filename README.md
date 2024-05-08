![kmp5](https://github.com/korei999/kmp2/assets/93387739/443b106a-ee1b-4f4a-afbd-1c748c4d65ca)

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

### Dependencies:
fedora: `sudo dnf install pipewire0.2-devel pipewire-devel ncurses-devel libsndfile-devel`
