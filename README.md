# Welcome to beat!os
This is an open-source 64-bit (x86_64) operating system created by me! (i used chatgpt for reference.)
Also, it MAY be able to run DOOM (if someone ported it properly).
# Building Prerequisites
1) Make (you can use this in docker if you're on windows)
2) ncurses (on debian the package is libncurses-dev) (same here)
3) Docker
4) Any x86_64 supported virtualization software (Make sure the VM is configured to x64-bit! THIS IS NOT BASED ON UNIX!)
# Building on Unix-Like Systems (ex. Linux):
1) Run start.sh as sudo in the project directory.
`sudo ./start.sh`
2) To build beat!os, tpye this command:
`make x86_64`
3) Exit the Docker machine.
4) Run vmstart.sh, still in the project directory:
`./vmstart.sh`
# Building on Windows:
1) Run start.bat in an administrator CMD window in the project directory.
`start.bat`
2) To build beat!os, type this command:
`make x86_64`
3) Exit the Docker machine.
4) Run vmstart.bat, still in the project directory:
`vmstart.bat`
# EXTRAS:
Use `make clean` to clean the build if needed.
