#!/bin/sh

YASM=`which yasm`

if [ -z $YASM ]
then
  echo "Could not find YASM!  Exiting."
  exit -1
fi

echo
echo Using yasm: `ls $YASM`

echo
echo "*** Building the bootsector ***"
./build_asm.sh $YASM ../fd bs
./build_asm.sh $YASM ../fd bs_2

echo
echo "*** Building Start ***"
./build_asm.sh $YASM ../fd Start

