#! /bin/bash
qemu-system-arm -M virt -m 512 -kernel stock-zImage -nographic \
  -drive file=rootfs.img,if=none,format=raw,id=rfs \
  -device virtio-blk-device,drive=rfs \
  -append "earlyprintk=serial root=/dev/vda"