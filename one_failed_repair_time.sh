#!/bin/bash

[ "$#" -lt 5 ] && echo "Usage: bash one_failed_repair_time.sh (new device) (fail device id) (repair mode) (result file path) (circle num) " && exit 0


for((i=1;i<$5+1;i=i+1))
do
	./recover $1 $2 $3 | grep "Repair_time" | awk '{print $3}' >> $4
done

