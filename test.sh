#!/bin/bash

# generate one img and mount to /dev/loop*, and generate a testfile

[ "$#" -lt 3 ] && \
	echo "Usage: bash test.sh (number of img files) (size of disk) (size of testfile(MB))" &&\
	exit 0

dn=$(ls | grep img | wc -l)
for(( i=1; i<$1+1; i++ ))
do
	losetup -d /dev/loop"$[$i+$dn]"
	dd if=/dev/zero of=file"$[$i+$dn]".img bs=1M count=$2
	losetup /dev/loop"$[$i+$dn]" file"$[$i+$dn]".img
done

dd if=/dev/urandom of=testfile bs=1M count=$3