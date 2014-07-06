#!/bin/bash

dn=$(ls | grep img | wc -l)
if [ $dn -ne 0 ]; then
	for((i=1;i<$dn+1;i=i+1))
	do
		losetup -d /dev/loop$i
	done
	echo "all the IMG files have been umount!"

	rm *.img
	echo "all the IMG files have been deleted!"
fi

:> raid_setting
echo "clear raid_setting"

:> raid_metadata
echo "clear raid_metadata"

:> raid_health
echo "clear raid_health"
