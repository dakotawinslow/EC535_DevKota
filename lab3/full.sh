#! /bin/bash
# Dakota Winslow 2025

./mod-rootfs.sh || { echo "mod-rootfs.sh failed"; exit 1; }
./rebuild-rootfs.sh || { echo "rebuild-rootfs.sh failed"; exit 1; }
./run.sh || { echo "run.sh failed"; exit 1; }