![kmp6](https://github.com/korei999/kmp2/assets/93387739/6aaa5c42-ba4a-4468-ae41-74a351e3678d)

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
fedora: `sudo dnf install pipewire0.2-devel pipewire-devel ncurses-devel libsndfile-devel`
