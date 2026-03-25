FROM debian:trixie-slim

RUN apt-get clean && apt-get update -o Acquire::ForceIPv4=true && apt-get install -y kmod udev sudo

RUN apt-get update && apt-get install -y \
    parted \
    grub-pc-bin \
    grub-efi-amd64-bin \
    dosfstools \
    e2fsprogs \
    util-linux \
    cpio \
    zip \
    qemu-system-x86 \
    ovmf \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /beatos