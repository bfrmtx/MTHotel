# versions

I use clang 13 or gcc 11.2. The C++ compiler must be (almost) C++20 compliant, cmake 3.20 and above.

# install

as sudo or root (sudo su) install

`apt update`

`apt install clang cmake libboost-dev sqlite3 sqlite3-tools libsqlite3-dev git`

(takes a few minutes, no reboot required, software git maybe already installed)

# build

If you don't want to edit any files, use this directory structure:

`mkdir -p $HOME/devel/github_mthotel`

`mkdir -p $HOME/build`

`sudo mkdir -p /usr/local/mthotel`

`sudo chown $USER:$USER /usr/local/mthotel`

`cd  $HOME/devel/github_mthotel`

`git clone https://github.com/bfrmtx/MTHotel.git`

`cd  $HOME/devel/github_mthotel/MTHotel/cpp`

`zsh clang_build_release_bfr.sh`

(bash ... in case you use bash)

in .bashrc or .zshrc add lines

`export PATH=$PATH:/usr/local/mthotel/bin`

`export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/mthotel/lib:/usr/local/procmt`

... and off you go! Under *WSL`your C:\ should be found under /mnt/c



