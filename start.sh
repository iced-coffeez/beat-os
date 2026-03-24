qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file=./OVMF_VARS.fd \
  -hda beatos.img \
  -m 4G \
  -enable-kvm \
  -netdev user,id=net0 \
  -device virtio-net-pci,netdev=net0 \