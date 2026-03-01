# beat!os
## Description:
hello! i am the creator of beat!os :D its based on the Linux Kernel (much thanks Linus Torvalds and the Linux Foundation) from [kernel.org](https://kernel.org).

# Prequisites
## Getting a JVM
download Oracle GraalVM 25 (JDK 25) from [graalvm.org](https://www.graalvm.org/downloads/), extract it, and place the contents in `files/jvm/` :P
## Installing neccessary packages
Dependencies:
```bash
parted grub-install mkfs.vfat mkfs.ext4 losetup cpio dd zip```
### Install Command for Debian/Ubuntu:
```bash
sudo apt install parted grub-pc grub-efi dosfstools e2fsprogs util-linux cpio zip
```