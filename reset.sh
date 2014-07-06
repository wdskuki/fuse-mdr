#!/bin/bash

# Run this script as root if not already.
chk_root () {

  if [ ! $( id -u ) -eq 0 ]; then
	echo "Please run as root"
	exit 0
#    echo "Please enter root's password."
#    exec su -c "${0} ${CMDLN_ARGS}" # Call this prog as root
#    exit ${?}  # sice we're 'execing' above, we wont reach this exit
               # unless something goes wrong.
  fi

}

chk_root

read -p "Are you sure want to return to the original state (Y/N)?" yn

if [ "$yn" == "Y" ] || [ "$yn" == "y" ] || [ "$yn" == "yes" ] || [ "$yn" == "YES" ]; then
	ls | grep -v sh | grep -v Makefile \
	   | grep -v README | grep -v xml \
	   | grep -v src | grep -v doc \
       | grep -v settings_template \
       | xargs rm -r
elif [ "$yn" == "N" ] || [ "$yn" == "n" ] || [ "$yn" == "no" ] || [ "$yn" == "NO" ]; then
	exit 0
else 
	echo "Please input corrent choice!"
fi
