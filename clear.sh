#!/bin/bash

[ "$#" -lt 1 ] && echo "Usage: bash clear.sh (number of disks)" && exit 0

rm file*.img

for((i=1;i<$1+1;i=i+1))
do
	losetup -d /dev/loop$i
done
