# Environment Setup

Thanks to the magic of .gitignore, you should be able to put a rootfs directory and smlinks to stock-linux and stock-zimage in this folder without them getting added to the git repo.

Usage:
1. `./mod-rootfs.sh`: Makes the `/km` and `/ul` folders, dumps their products in rootfs/root as well as any scripts in `/sh`
2. `./rebuild-rootfs.sh`: Makes the current rootfs into an .img 
3. `./run.sh`: Starts QEMU with appropriate parameters