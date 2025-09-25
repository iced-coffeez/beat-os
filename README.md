# Welcome to beat!os
This is an open-source 64-bit (x86_64) operating system created by me! (i used chatgpt for reference.)
Also, it MAY be able to run DOOM (if someone ported it properly).
# Building Prerequisites
1) Make (you can use this in docker if you're on windows)
2) ncurses (on debian the package is libncurses-dev) (same here)
3) Docker
4) Any x86_64 supported virtualization software (Make sure the VM is configured to x64-bit! THIS IS NOT BASED ON UNIX!)
# Building on Unix-Like Systems (ex. Linux):
1) Run start.sh as sudo.
`sudo ./start.sh`
2) Once you're in the virtual machine (or after it is done building), type this command:
`make x86_64`
3) Exit the virtual machine.
4) Run vmstart.sh:
`./vmstart.sh`
# Building on 
# EXTRAS:
Use `make clean` to clean the build if needed.
