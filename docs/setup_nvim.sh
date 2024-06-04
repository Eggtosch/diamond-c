DIR=$(realpath $(dirname "$0"))
cd $DIR

cp -u dm.vim ~/.config/nvim/ftdetect/
cp -u diamond.vim ~/.config/nvim/after/syntax/
cp -u diamond.lua ~/.config/nvim/after/ftplugin/
cp -u diamond ~/.local/share/bash-completion/completions/
