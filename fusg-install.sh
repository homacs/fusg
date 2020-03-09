#!/bin/bash


if ! [ "$USER" == "root" ]
then
	sudo "$0" "$@"
	exit $?
fi


# actual installation starts here


EXE="fusg"

FUSG_SOURCE="./Debug/$EXE"
FUSG_TARGET="/usr/local/bin/$EXE"

if ! [ -e "$FUSG_SOURCE" ]
then
	echo "error: file '$FUSG_SOURCE' not found - abort." >2
	exit 1
fi

cp "$FUSG_SOURCE" "$FUSG_TARGET"
chmod +s "$FUSG_TARGET"


