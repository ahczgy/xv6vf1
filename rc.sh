#!/bin/bash

if [ `ps -aux | grep sshd | wc -l` -eq 1 ]; then
	service ssh start
else 
	echo "sshed has started...."
fi

if [ `pwd` = "/c/xv6vf2" ]; then 

	echo "In /c/xv6vf2"
else 
	cd /c/xv6vf2
fi
