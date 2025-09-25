#!/bin/bash
if [ "$(id -u)" -ne 0 ]; then
   echo "This script needs to be ran with sudo, or as a root user."
   exit
fi

sudo docker run --rm -it -v $(pwd):/root/env beat-buildenv