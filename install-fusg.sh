#!/bin/bash
#
#
# Installs command line tool fusg and daemon fusgd.
#
#   The important part is:
#
#   * to be root and
#   * chmod +s fusg
#
#   ... otherwise, fusg can't access the db.
#


if ! [ "$USER" == "root" ]
then
	sudo "$0" "$@"
	exit $?
fi


# actual installation starts here

FUSG_SOURCES=/home/homac/repos/git/cakelab.org/playground/fusg
FUSG_BUILD=${FUSG_SOURCES}/Debug


AUDISP_PLUGINS_DIR="/etc/audisp/plugins.d"

FUSG_ETC_DIR="/etc/fusg"
FUSG_VAR_DIR="/var/fusg"

FUSGD_PLUGIN_CONF_SOURCE="${FUSG_SOURCES}/etc/audisp/plugins.d/fusgd.conf"
FUSGD_PLUGIN_CONF_TARGET="${AUDISP_PLUGINS_DIR}/fusgd.conf"
FUSG_CONF="${FUSG_SOURCES}/etc/fusg/fusg.conf"




FUSG_EXE="fusg"
FUSG_SOURCE="${FUSG_BUILD}/$FUSG_EXE"
FUSG_TARGET="/usr/local/bin/$FUSG_EXE"
if ! [ -e "$FUSG_SOURCE" ]
then
	echo "error: file '$FUSG_SOURCE' not found - abort." >2
	exit 1
fi



FUSGD_EXE="fusgd"
FUSGD_SOURCE="${FUSG_BUILD}/$FUSGD_EXE"
FUSGD_TARGET="/sbin/$FUSGD_EXE"
if ! [ -e "$FUSGD_SOURCE" ]
then
	echo "error: file '$FUSGD_SOURCE' not found - abort." >2
	exit 2
fi



[ -e "$FUSG_TARGET" ] && echo "already exists: '$FUSG_TARGET'" && exit 3
[ -e "$FUSGD_TARGET" ] && echo "already exists: '$FUSGD_TARGET'" && exit 4
[ -e "$FUSG_ETC_DIR" ] && echo "already exists: '$FUSG_ETC_DIR'" && exit 5
[ -e "$FUSG_VAR_DIR" ] && echo "already exists: '$FUSG_VAR_DIR'" && exit 6
[ -e "$FUSGD_PLUGIN_CONF_TARGET" ] && echo "already exists: '$FUSGD_PLUGIN_CONF_TARGET'" && exit 7



echo "stopping auditd .."
systemctl stop auditd || exit 8


echo "installing configs and executables .."
mkdir -p "${FUSG_ETC_DIR}" || exit 9
mkdir -p "${FUSG_VAR_DIR}" || exit 10
cp "$FUSG_CONF" "${FUSG_ETC_DIR}" || exit 11 
cp "$FUSGD_PLUGIN_CONF_SOURCE" "${FUSGD_PLUGIN_CONF_TARGET}" || exit 12 

cp "$FUSGD_SOURCE" "$FUSGD_TARGET" || exit 13

cp "$FUSG_SOURCE" "$FUSG_TARGET"  || exit 14
chmod +s "$FUSG_TARGET"           || exit 15


echo "starting auditd .."
systemctl start auditd   || exit 16

echo "done."



