#! /bin/bash
# Dakota Winslow 2025

mknod /dev/mytimer c 61 0
insmod /root/mytimer.ko

# ./ktimer -s 300 dakota
# ./ktimer -l

# sleep 3

# ./ktimer -s 300 billy
# ./ktimer -l

# sleep 1

# ./ktimer -m 5
# ./ktimer -s 300 billy
# ./ktimer -l

# sleep 1

# ./ktimer -s 20 dakota
# ./ktimer -l