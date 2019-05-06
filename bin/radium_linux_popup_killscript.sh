#!/bin/bash

# X often freezes if a program crash while a popup menu is open. (at least when running under gdb)


echo $$ >/tmp/$1

sleep $2

if [ -f /tmp/$1 ] ; then
    echo "killing it: $1 $2 $3"
    kill -9 $3
fi

