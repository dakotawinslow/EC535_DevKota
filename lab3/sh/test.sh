#! /bin/bash
# Dakota Winslow 2025

mknod /dev/mytimer c 61 0
insmod /root/mytimer.ko

echo "TEST: Help message:"
./ktimer -h

echo "TEST: Empty timer list:"
./ktimer -l

echo "TEST: Set a 2s timer called bob:"
./ktimer -s 2 bob
./ktimer -l

sleep 3

echo "TEST: Set a 30s timer called billy:"
./ktimer -s 30 billy
./ktimer -l

sleep 1

echo "TEST: Modify the 30s timer to 2s:"
./ktimer -s 2 billy
./ktimer -l


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