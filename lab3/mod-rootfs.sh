#! /bin/bash
# Dakota Winslow 2025
# run this before building image to make everythingh and add to rootfs
# make sure that $WORKSPACE = /what/ever/path/EC535_DevKota/lab3
# Add in kernel modules

cd $WORKSPACE/km
make clean
make
cd $WORKSPACE/ul
make clean
make

cd $WORKSPACE
cp $WORKSPACE/km/*.ko $WORKSPACE/rootfs/root/
cp $WORKSPACE/sh/*.sh $WORKSPACE/rootfs/root/
cp $WORKSPACE/ul/ktimer $WORKSPACE/rootfs/root/ktimer


# Clean up
cd $WORKSPACE/km
make clean

cd $WORKSPACE/ul
make clean

cd $WORKSPACE

