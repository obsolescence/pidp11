#!/bin/bash
#
# copies the blinkenlightd server demon to beaglebone and does an installation.

export REMOTE_INSTALLATION_SCRIPT=/tmp/blinkenlight_inst_temp.sh

# blinkenlight daemon is installed to run these configurations.
# Can be changed on beaglebone by replacing /etc/blinkenlightd.conf

## Use all configs defined in subdir "panels"
#CONFIGURATION_FILELIST="`find ~/retrocmp/blinkenbone/panels -name '*.conf' `"
# CONFIGURATION_FILELIST=~/beaglebone/panels/pdp11-40/pdp11-40.conf
if [ $# -ne 1 ] ; then
	echo "usage: $0 <blinkenlight-config-file>"
	echo "examples:"
	for f in ~/retrocmp/blinkenbone/panels/*/*.conf
	do
		echo "    \$ $0 $f"
	done
  exit 1
fi
CONFIGURATION_FILELIST=$1


### 0) build it
export MAKE_TARGET_ARCH=BEAGLEBONE
make clean
make blinkenlightd
# binary in blinkenlightd




### 1) prepare remote script
echo "--- Create installation script $REMOTE_INSTALLATION_SCRIPT.sh, to run on BeagleBone"

# Create here a sh script to run on beaglebone
# $var and `` are normally evaluated at script creation
# \$var and \`cmd\` are evaluated on script run
#
cat <<!!! >$REMOTE_INSTALLATION_SCRIPT

echo --- Install RPC portmapper
opkg install portmap

# log output of blinkenlightd in /var/log/user
grep 'user.\*' /etc/rsyslog.conf  >/dev/null
if [ $? -ne 0 ] ; then
echo "user.*         /var/log/user" >>/etc/rsyslogd.conf
echo "!!! /etc/rsyslogd.conf was changed"
echo "!!! You need to REBOOT to apply these changed."
fi

# just copy demon too home/root for manual start ... later we will run at startup
mv /tmp/blinkenlightd	~

echo "(re)installing startup blinkenlightd link /etc/rc*.d"
echo "& (re)starting blinkenlightd daemon"
/etc/init.d/blinkenlightd stop
/usr/sbin/update-rc.d -f blinkenlightd remove

mv /tmp/blinkenlightd.init /etc/init.d/blinkenlightd
chmod a+x  /etc/init.d/blinkenlightd
/usr/sbin/update-rc.d blinkenlightd defaults

mv /tmp/blinkenlightd.conf /etc/blinkenlightd.conf
# start blinkenlighd the first time
# nohup does not work .. daemon is killed on ssh logout?
# nohup ~/blinkenlightd -c /etc/blinkenlightd.conf -b &
!!!

### 1) set time
ssh ${REMOTEINSTALL_REMOTE_SSH_USERHOST} "date -u `date -u +%m%d%H%M%Y.%S`; echo `cat /etc/timezone` > /etc/timezone"

#### 2) transfer files to BeagleBone
echo "--- install and operate kernel module \"$THEMODULE\" on Beaglebone"
echo "--- set remote time"
ssh ${REMOTEINSTALL_REMOTE_SSH_USERHOST} "date -u `date -u +%m%d%H%M%Y.%S`; echo `cat /etc/timezone` > /etc/timezone"

echo "Copy module and installation scripts ..."
cat $CONFIGURATION_FILELIST >/tmp/blinkenlightd.conf
scp /tmp/blinkenlightd.conf  ${REMOTEINSTALL_REMOTE_SSH_USERHOST}:/tmp/blinkenlightd.conf
scp bin-beaglebone/blinkenlightd blinkenlightd.init $REMOTE_INSTALLATION_SCRIPT ${REMOTEINSTALL_REMOTE_SSH_USERHOST}:/tmp

echo "Remote execute installation script on beaglebone ..."
ssh ${REMOTEINSTALL_REMOTE_SSH_USERHOST} "chmod +x $REMOTE_INSTALLATION_SCRIPT ; /bin/sh $REMOTE_INSTALLATION_SCRIPT"

echo "Rebooting ...  will start service"
# "nohup", so ssh returns, but reboot takes place
ssh ${REMOTEINSTALL_REMOTE_SSH_USERHOST} "nohup /sbin/reboot &"

echo "Wait reboot time ..."
sleep 15
echo "... should be up now again!"





