# Welcome to beat!os
This is an open-source 64-bit (x86_64) operating system created by me! (i used chatgpt for reference.)
Also, it MAY be able to run DOOM (if someone ported it properly).
# Building Prerequisites
Make
ncurses (on debian the package is libncurses-dev)
Docker
Qemu (you can install that in the docker, or on your main machine.)
# 
Linux or a Unix-like system is preferrred.
# 
# Building:
1) Run start.sh as sudo.
`sudo ./start.sh`
2) Once you're in the virtual machine (or after it is done building), type this command:
`make x86_64`
3) Exit the virtual machine.
4) Run vmstart.sh:
`./vmstart.sh`
# EXTRAS:
Use `make clean` to clean the build if needed.

# DISCLAIMER:
I do not have a guide on how to build this on Windows or macOS, unfortunately. macOS should work similarly to Linux, since they are both built on Unix. (I know macOS is XNU, but it's similar.) I say this because start.sh is required to start the docker.
