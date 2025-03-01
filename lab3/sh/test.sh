#! /bin/bash
# Dakota Winslow 2025

mknod /dev/mytimer c 61 0
insmod /root/mytimer.ko


echo "TEST: Help message:"
./ktimer -h

echo "TEST: cat /dev/mytimer:"
cat /dev/mytimer

echo "TEST: cat /proc/mytimer:"
cat /proc/mytimer

echo "TEST: Empty timer list:"
./ktimer -l

echo "TEST: Set a 2s timer called bob:"
./ktimer -s 2 bob &
echo "TEST: Listing timers:"
./ktimer -l
echo "Sleeping 1..."
sleep 1 
echo "TEST: Listing timers:"
./ktimer -l

echo "Sleeping 3..."
sleep 3

echo "TEST: Set a 30s timer called billy:"
./ktimer -s 30 billy &
echo "TEST: Listing timers:"
./ktimer -l

echo "Sleeping 1..."
sleep 1

echo "TEST: Modify the 30s timer to 2s:"
./ktimer -s 2 billy &
echo "TEST: Listing timers:"
./ktimer -l
