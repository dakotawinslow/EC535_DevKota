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

echo "Sleeping 3..."
sleep 3

echo "TEST: Set a 5s timer called joe:"
./ktimer -s 5 joe &
echo "TEST: Listing timers:"
./ktimer -l

echo "Removing all timers..."
./ktimer -r

echo "TEST: Listing timers:"
./ktimer -l

echo "adding second timer..."
./ktimer -m 2

sleep 1

echo "Setting timer A for 10s..."
./ktimer -s 5 A &
echo "Setting timer B for 5s..."
./ktimer -s 3 B &

sleep 1

echo "TEST: Listing timers:"
./ktimer -l


