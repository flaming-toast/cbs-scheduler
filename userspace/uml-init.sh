#!/bin/bash

# UML Invocation:
# ./linux/linux rootfstype=hostfs rw init=/bin/bash mem=512M

losetup /dev/loop0 .lpfs/disk.img
mount -t lpfs /dev/loop0 /mnt
