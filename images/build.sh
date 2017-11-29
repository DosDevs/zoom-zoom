#!/bin/sh

NEWFS=../newfs/NewFs

echo
echo Using NewFs: `ls $NEWFS`

echo
echo "*** Making a 2.88MB FAT12 Disk Image ***"
$NEWFS fd.img ../fd/bs ../fd/Start ../fd/Elfo ../fd/Memo

