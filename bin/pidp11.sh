#!/bin/bash

# for SDL2 graphics
export DISPLAY=:0
xhost +
cd /opt/pidp11/bin
# Make a temp directory in the /run ram disk (if it's not already created)
sudo mkdir /run/pidp11 2>/dev/null
sudo chmod a+rw /run/pidp11
export XDG_RUNTIME_DIR=/run/pidp11
# VT-100
#oldterm=$TERM
#export TERM=VT100

echo "*** Start portmapper for RPC service, OK to fail if already running"
sudo rpcbind & 
sleep 2

while
	# Kill possibly still running instances of Blinkenlight server ... only one allowed
	sudo kill `pidof server11` >/dev/null 2>/dev/null
	#sleep 2


	if [ "$#" -eq 1 ]; then
		echo "Ignoring SR switches, argument provided to script:" $1
		lo=$1
	else
		# get SR switches from scansw
		sw=`sudo ./scansw`
		# get low 18 bits and high 4 bits
		#hi=`expr $sw / 262144`
		lo=`expr $sw % 262144`
	fi
	# format as octal
	# lo identifies bootscrtipt. hi is meant for future special functions
	lo=`printf "%04o" $lo`
	sel=`/opt/pidp11/bin/getsel.sh $lo | sed 's/default/idled/'`
	echo "*** booting $sel ***"
	# create a bootscript for simh in the /run ramdisk:
	(echo cd /opt/pidp11/systems/$sel;
	echo do boot.ini
	) >/run/pidp11/tmpsimhcommand.txt
	echo "*** Start client/server ***"
	sudo -E ./server11 &
	sleep 2
	sudo -E ./client11 /run/pidp11/tmpsimhcommand.txt
	
	# after simh exits, check if a newly created command file now says exit (meaning pls reboot)
	if [[ $(< /run/pidp11/tmpsimhcommand.txt) == "exit" ]]; then
		reboot=1
	else
		reboot=0
	fi

	((reboot==1))
do
	:
done
	# Kill Blinkenlight server ... exiting
	sudo kill `pidof server11` >/dev/null 2>/dev/null
	# Delete tmp simh command file
	sudo rm /run/pidp11/*.txt
	
	#export TERM=$oldterm
