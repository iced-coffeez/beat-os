#!/bin/bash
set -euo pipefail

bt="$1"

cleanup() {
	echo "Cleaning up..."

	for dir in dev proc sys run; do
		if mountpoint -q "/var/tmp/beatos/$dir" 2>/dev/null; then
			sudo umount "/var/tmp/beatos/$dir" || echo "Failed to unmount /var/tmp/beatos/$dir"
		fi
	done

	if mountpoint -q /var/tmp/beatos/boot/efi 2>/dev/null; then
		sudo umount /var/tmp/beatos/boot/efi || echo "Failed to unmount /var/tmp/beatos/boot/efi"
	fi

	if mountpoint -q /var/tmp/beatos 2>/dev/null; then
		sudo umount /var/tmp/beatos || echo "Failed to unmount /var/tmp/beatos"
	fi

	if [[ -n "${LOOP_DEV:-}" ]]; then
		sudo losetup -d "$LOOP_DEV" || echo "Failed to detach $LOOP_DEV"
		echo "Detached device $LOOP_DEV"
	else
		echo "No loop device to detach"
	fi

	sudo rm -rf /var/tmp/beatos 2>/dev/null || true

	rm initramfs.cpio
}

trap cleanup EXIT INT TERM

echo "Compressing initramfs..."

if [ -e initramfs.cpio ]; then
	rm initramfs.cpio
	echo "Cleaned old initramfs"
fi

cd initramfs

cp ../.version .

find . | cpio -o -H newc | gzip > ../initramfs.cpio

cd ..

echo "Finding required size for image..."
BOOT_SIZE=$(du -sb bzImage initramfs.cpio jvm | awk '{sum+=$1} END {print sum}')
TOTAL_SIZE=$BOOT_SIZE
OVERHEAD=$((TOTAL_SIZE / 4))
IMAGE_SIZE=$((TOTAL_SIZE + OVERHEAD + 536870912))
IMAGE_SIZE_MB=$(( (IMAGE_SIZE / 1048576) + 1 ))
echo "..Done!"

echo "Creating image... [${IMAGE_SIZE_MB}MB]"

if [ -e beatos.img ]; then
	rm beatos.img
	echo "Cleaned old image"
fi

dd if=/dev/zero of=beatos.img bs=1M count=${IMAGE_SIZE_MB} status=progress && sync

echo "Created a ${IMAGE_SIZE_MB}MB image."

echo "Partitioning image with parted ..."
sudo parted beatos.img --script \
	mklabel gpt \
    mkpart BIOSBOOT 1MiB 4MiB \
    set 1 bios_grub on \
    mkpart ESP fat32 4MiB 68MiB \
    set 2 esp on \
    mkpart ROOT ext4 104MiB 100%

echo "Setting up loop device..."
LOOP_DEV=$(sudo losetup -Pf --show "beatos.img")
echo "Loop device is $LOOP_DEV"

sleep 1

EFI_DEV="${LOOP_DEV}p2"
ROOT_DEV="${LOOP_DEV}p3"

sudo partprobe "$LOOP_DEV" || true

echo "Formatting image..."
sudo mkfs.vfat -F32 "$EFI_DEV"
sudo mkfs.ext4 -L "beatOS" "$ROOT_DEV" -O ^has_journal

echo "Mounting at /var/tmp/beatos ..."
sudo mkdir -p /var/tmp/beatos
# sudo mount "$PART_DEV" /var/tmp/beatos
sudo mount "$ROOT_DEV" /var/tmp/beatos
sudo mkdir -p /var/tmp/beatos/boot/efi
sudo mount "$EFI_DEV" /var/tmp/beatos/boot/efi

echo "Creating directories..."

sudo mkdir -p /var/tmp/beatos/{dev,proc,sys,run,tmp,lib,etc,sbin,opt,usr,mnt}

echo "Copying contents..."

sudo mkdir -p /var/tmp/beatos/boot
sudo cp bzImage /var/tmp/beatos/boot/vmlinuz-beatOS
sudo cp initramfs.cpio /var/tmp/beatos/boot/initramfs-beatOS.cpio.gz

sudo mkdir -p /var/tmp/beatos/lib /var/tmp/beatos/lib64
sudo ldd jvm/bin/java | grep "=>" | awk '{print $3}' | sudo xargs -I '{}' cp -v '{}' /var/tmp/beatos/lib
find jvm/lib -name "*.so*" -type f | while read lib; do
    sudo ldd "$lib" 2>/dev/null | grep "=>" | awk '{print $3}' | sudo xargs -I '{}' cp -v '{}' /var/tmp/beatos/lib/ 2>/dev/null || true
done

sudo cp /lib64/ld-linux-x86-64.so.2 /var/tmp/beatos/lib64/ 2>/dev/null || \
  sudo cp /lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 /var/tmp/beatos/lib64/

sudo mkdir /var/tmp/beatos/opt/jvm
sudo mkdir /var/tmp/beatos/opt/apps

sudo cp -r jvm/. /var/tmp/beatos/opt/jvm

sudo cp -r apps/. /var/tmp/beatos/opt/apps

# sudo ldd apps/runner | awk '{if (match($3,"/")) print $3}' | sudo xargs -I '{}' cp -v '{}' /tmp/beatos/lib/

# sudo mkdir -p /tmp/beatos/opt/{jvm,apps}
# sudo cp -a jvm/. /tmp/beatos/opt/jvm/
# sudo cp -a apps/. /tmp/beatos/opt/apps

cat <<'EOF' | sudo tee /var/tmp/beatos/etc/fstab >/dev/null
LABEL=beatOS  /  ext4  defaults  0  1
EOF

cat <<'EOF' | sudo tee /var/tmp/beatos/etc/passwd >/dev/null
root:x:0:0:root:/root:/bin/sh
EOF

cat <<'EOF' | sudo tee /var/tmp/beatos/etc/group >/dev/null
root:x:0:
EOF

if [ $# -eq 0 ]; then
	read -p "'dev' build or 'prod' build? (dev/prod): " bt
fi

bt=$(echo "$bt" | tr '[:upper:]' '[:lower:]')

echo "Making /sbin/init ..."

# CREATING /sbin/init #

if  [ "$bt" == "prod" ]; then
cat <<'EOF' | sudo tee /var/tmp/beatos/sbin/init >/dev/null
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
mount -t tmpfs tmpfs /tmp

if [ ! -e /bin/java ]; then
	ln -s /opt/jvm/bin/java /bin/java
fi

export ENV=/etc/profile

if [ ! -f /etc/profile ]; then
	echo "alias shutdown='sync; poweroff -f'" > /etc/profile
	echo "alias reboot='sync; reboot -f'" >> /etc/profile
fi

mkdir -p /tmp/lower_root /tmp/overlay /mnt/root

cp -a /bin /sbin /lib /lib64 /opt /etc /usr /mnt /tmp/lower_root
mount -t tmpfs -o size=1024M tmpfs /tmp/overlay
mkdir -p /tmp/overlay/upper /tmp/overlay/work
mount -t overlay overlay -o lowerdir=/tmp/lower_root,upperdir=/tmp/overlay/upper,workdir=/tmp/overlay/work /mnt/root

mkdir -p /mnt/root/dev \
	/mnt/root/proc \
	/mnt/root/sys \
	/mnt/root/run \
	/mnt/root/tmp \
	/mnt/root/lib \
	/mnt/root/etc \
	/mnt/root/sbin \
	/mnt/root/opt \
	/mnt/root/usr \
	/mnt/root/mnt

mount -t proc proc /mnt/root/proc
mount -t sysfs sysfs /mnt/root/sys
mount -t devtmpfs devtmpfs /mnt/root/dev
mount -t tmpfs tmpfs /mnt/root/run
mount -t tmpfs tmpfs /mnt/root/tmp

echo "beat!os (Installation Media)"
chroot /mnt/root /bin/sh -c "exec setsid cttyhack /bin/sh"

sync
poweroff -f
EOF
fi

# DEV BUILD
if [ "$bt" != "prod" ]; then
cat <<'EOF' | sudo tee /var/tmp/beatos/sbin/init >/dev/null
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t tmpfs tmpfs /run
mount -t tmpfs tmpfs /tmp

if [ ! -e /bin/java ]; then
	ln -s /opt/jvm/bin/java /bin/java
fi

export ENV=/etc/profile

if [ ! -f /etc/profile ]; then
	echo "alias shutdown='sync; poweroff -f'" > /etc/profile
	echo "alias reboot='sync; reboot -f'" >> /etc/profile
	echo "export TERM=xterm-256color" >> /etc/profile
	echo "export SHELL=/bin/sh" >> /etc/profile
fi

echo "beat!os (Live Disk)"
setsid cttyhack /bin/sh
sync
poweroff -f
EOF
fi
sudo chmod +x /var/tmp/beatos/sbin/init

# echo "Preparing for chroot ..."
# sudo mount --bind /dev /tmp/beatos/dev
# sudo mount --bind /proc /tmp/beatos/proc
# sudo mount --bind /sys /tmp/beatos/sys
# sudo mount --bind /run /tmp/beatos/run

echo "Installing GRUB..."

echo "Installing GRUB [1/2...] (bios)"
sudo grub-install \
	--target=i386-pc \
	--boot-directory=/var/tmp/beatos/boot \
	--modules="part_gpt ext2" \
	"$LOOP_DEV"
echo "Installing GRUB [2/2...] (uefi)"
sudo grub-install \
  --target=x86_64-efi \
  --efi-directory=/var/tmp/beatos/boot/efi \
  --boot-directory=/var/tmp/beatos/boot \
  --removable \
  --recheck
 
# sudo grub-install --no-floppy --root-directory=/tmp/beatos "$LOOP_DEV"

sudo mkdir -p /var/tmp/beatos/boot/grub
sudo mkdir -p /var/tmp/beatos/boot/efi/boot/grub

cat <<'EOF' | sudo tee /var/tmp/beatos/boot/grub/grub.cfg >/dev/null
set timeout=5
set default=0

menuentry 'beat!os Installer' {
    linux   /boot/vmlinuz-beatOS root=LABEL=beatOS rw console=tty1
    initrd  /boot/initramfs-beatOS.cpio.gz
}
EOF

# UEFI crap
cat <<'EOF' | sudo tee /var/tmp/beatos/boot/grub/grub_UEFI.cfg >/dev/null
set timeout=5
set default=0

menuentry 'beat!os Installer' {
	linux /boot/vmlinuz-beatOS root=LABEL=beatOS rw console=tty1
	initrd /boot/initramfs-beatOS.cpio.gz
}
EOF

cat <<'EOF' | sudo tee /var/tmp/beatos/boot/efi/boot/grub/grub.cfg >/dev/null

search --label beatOS --set=root
set prefix=($root)/boot/grub
configfile /boot/grub/grub_UEFI.cfg

EOF

# echo "Sorry, this installer is still under construction! Unmount /tmp/beatOS and remove $LOOP_DEV"

echo "beatOS (v$(cat .version)) installation image compiled."