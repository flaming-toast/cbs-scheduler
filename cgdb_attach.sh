#!/bin/bash

sleep 5s ; 
cgdb $(readlink -f linux/vmlinux) \
	-ex "target remote :1234"
