#!/bin/bash

rm -rf libcopy

sudo ldd jvm/bin/java | grep "=>" | awk '{print $3}' | sudo xargs -I '{}' cp -v '{}' libcopy/lib
find jvm/lib -name "*.so*" -type f | while read lib; do
    sudo ldd "$lib" 2>/dev/null | grep "=>" | awk '{print $3}' | sudo xargs -I '{}' cp -v '{}' libcopy/lib/ 2>/dev/null || true
done

mkdir -p libcopy/{lib,lib64}

sudo cp /lib64/ld-linux-x86-64.so.2 libcopy/lib64/ 2>/dev/null || \
  sudo cp /lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 libcopy/lib64/
