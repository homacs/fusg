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



AUDISP_PLUGINS_DIR="/etc/audisp/plugins.d"

FUSG_ETC_DIR="/etc/fusg"
FUSG_VAR_DIR="/var/fusg"

FUSGD_PLUGIN_CONF_TARGET="${AUDISP_PLUGINS_DIR}/fusgd.conf"
FUSG_CONF="${FUSG_SOURCES}/etc/fusg/fusg.conf"


FUSG_EXE="fusg"
FUSG_TARGET="/usr/local/bin/$FUSG_EXE"

FUSGD_EXE="fusgd"
FUSGD_TARGET="/sbin/$FUSGD_EXE"




echo "stopping auditd .."
systemctl stop auditd || exit 3

echo "removing configs and executables .."

rm -f "$FUSG_TARGET"   || exit 4
rm -f "$FUSGD_TARGET" || exit 5
rm -rf "$FUSG_ETC_DIR" || exit 6
rm -rf "$FUSG_VAR_DIR" || exit 7
rm -f "$FUSGD_PLUGIN_CONF_TARGET" || exit 8


echo "starting auditd .."
systemctl start auditd   || exit 9

echo "done."



