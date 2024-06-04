# Diamond Programming Language

## neovim syntax highlighting

Copy the `docs/dm.vim` file to `~/.config/nvim/ftdetect/dm.vim`, so neovim will detect files
that end with `.dm` as diamond files.

Copy the `docs/diamond.vim` file to `~/.config/nvim/after/syntax/diamond.vim`, so neovim will
add the syntax rules to diamond files.

Copy the `docs/diamond.lua` file to `~/.config/nvim/after/ftplugin/diamond.lua`, so neovim will
start the diamond lsp server when a diamond file is opened.
One may has to adjust the command path the server is at.
