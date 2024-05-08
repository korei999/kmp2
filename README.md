![kmp4](https://github.com/korei999/kmp2/assets/93387739/4efc72ac-7af8-4518-a685-9e3ac65df406)

Supports: wav, opus, ogg, mp3, flac.
### Usage:
`kmp *`, `kmp **`, `kmp $(find . | rg "\.mp3|\.wav")` or whatever your shell can do.
Navigate with vim-like keybinds.
Searching with `/` or `?`, `n` and `N` to next-prev found string.
`9` and `0` volume.
`t` to set time like `4:20`, `40` or `60%`.
`:` to jump to selected number in the playlist.
`z` to center around currently playing song.

#### Dependencies:
fedora: `sudo dnf install pipewire0.2-devel pipewire-devel ncurses-devel libsndfile-devel`
