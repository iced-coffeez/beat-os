# Welcome to beat!os
This is a 64-bit (x86_64) operating system created by me! (protected under LGPL) (i used chatgpt for reference.)
Also, it MAY be able to run DOOM (if someone ported it properly).
# Building Prerequisites
1) Make (you can use this in docker if you're on windows)
2) Git (if you're cloning the repository. If you're downloading from a .zip, you don't need this.)
3) ncurses (on debian the package is libncurses-dev) (you can use this in docker. already installed by default, usually.)
4) Docker
5) elf-gcc (is included in Dockerfile)
6) Any x86_64 supported virtualization software (Make sure the VM is configured to x64-bit! THIS IS NOT BASED ON UNIX!)
# Automated Building + Cloning (on linux)
```
# OTHER METHODS ARE RECOMMENDED OVER THIS!
# update and upgrade packages before this
git clone https://github.com/iced-coffeez/beat-os.git
cd beat-os
sudo make x86_64 -j 8
./vmstart.sh
```
# Cloning (on Windows)
```
:: OTHER METHODS ARE RECOMMENDED OVER THIS! (ALSO: INSTALL DOCKER BEFORE RUNNING)
git clone https://github.com/iced-coffeez/beat-os.git
cd beat-os
dockerbuild.bat
start.bat
```
# Building on Unix-Like Systems (ex. Linux):
1) Update and Upgrade all packages.
2) Run start.sh as sudo in the project
directory.
`sudo ./start.sh`
3) To build beat!os, tpye this command:
`make x86_64`
4) Exit the Docker machine.
5) Run vmstart.sh, still in the project directory:
`./vmstart.sh`
# Building on Windows:
1) Make sure your system software is up to date. [PREFERRED, NOT REQUIRED]
2) Run start.bat in an administrator CMD window in the project directory.
`start.bat`
3) To build beat!os, type this command:
`make x86_64`
4) Exit the Docker machine.
5) Run vmstart.bat, still in the project directory:
`vmstart.bat`
# EXTRAS:
Use `make clean` to clean the build if needed.
