#!/bin/sh
#
# Usage: $0 path_to_yasm output_name
#
# Will try to find output_name.asm
#

COMMAND="$1 -o $2/$3 $3.asm"

echo Running $COMMAND
$COMMAND

