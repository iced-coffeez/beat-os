# beat!os
## Description:
hello! i am the creator of beat!os :D its based on the Linux Kernel (much thanks Linus Torvalds and the Linux Foundation) from [kernel.org](https://kernel.org).

# Prerequisites
YOU NEED AT LEAST 5GB OF STORAGE AVAILABLE!
## Getting a JVM
download Oracle GraalVM 25 (JDK 25) from [graalvm.org](https://www.graalvm.org/downloads/), extract it, and place the contents in `jvm/` :P
## Installing necessary packages
Dependencies:
```bash
parted grub-install mkfs.vfat mkfs.ext4 losetup cpio dd zip
```
OVMF is needed for ./start.sh, too. Just if you want to use Qemu in UEFI.

Install Command for Debian/Ubuntu:
```bash
sudo apt install parted grub-pc-bin grub-efi-amd64-bin dosfstools e2fsprogs util-linux cpio zip qemu-system-x86 ovmf
```
# Building and testing
## Building an image
Run `./image_creator.sh [dev|prod]`. You can also just run `./image_creator.sh` if you want it to prompt you for some reason.
## Building a package
Run `./packageBuilder.sh <package-folder> <private-key>` and make sure that your package folder looks somewhat like this:
```text
<package folder>
    .packageInfo
    bin/
    lib/
```

Make sure that `.packageInfo` contains:
```text
exec=
```
It can also contain version= or installHook= or lib= for libraries.

Example `.packageInfo`:
```text
exec=bin/helloWorld
version=0.0.1
installHook=test.hook
```
Example `test.hook`:
```bash
lang=shell
script=(echo "Hello, World!")
```
Example package folder:
```text
<package folder>
    .packageInfo
    bin/
    lib/
    test.hook
```

## Testing beat!os
If you haven't already, copy the OVMF_VARS.fd file over to the root of the Git repo.
```bash
cp /usr/share/OVMF/OVMF_VARS.fd .
```
If that doesn't work, try:
```bash
cp /usr/share/OVMF/OVMF_VARS_4M.fd OVMF_VARS.fd
```
After that, you can run:
`./start.sh`

## Testing a beat!os package after building
The `./image_creator.sh` script copies apps/* into /opt/apps, so just place your .pub file in apps/pkg and copy it to /etc/pkg/trusted.

Copy your .boxpkg file into the apps/pkg directory, and then run:
`./image_creator.sh [dev|prod]`

Then run `./start.sh` and try:
```bash
cd /opt/apps/pkg
java pkg unpack --local < your package here >.boxpkg
```