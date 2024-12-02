#!/bin/bash

# Init script for pidp11.  

#boot_number=$2
argc=$#
pidp11="/opt/pidp11/bin/pidp11.sh"
pidp_dir=`dirname $pidp11`
pidp_bin=`basename $pidp11`

# Requires screen utility for detached pidp10 console functionality.
#
#test -x /usr/bin/screen || ( echo "screen not found" && exit 0 )
#test -x $pidp11 || ( echo "$pidp11 not found" && exit 0 )

# Check if pidp10 is already runnning under screen.
#
is_running() {
	procs=`screen -ls pidp11 | egrep '[0-9]+\.pidp11' | wc -l`
	return $procs
}

do_stat() {
	is_running
	status=$?
	if [ $status -gt 0 ]; then
	    echo "PiDP-11 is up." >&2
	    return $status
	else
	    echo "PiDP-11 is down." >&2
	    return $status
	fi
}

 

do_start() {
	is_running
	if [ $? -gt 0 ]; then
	    echo "PiDP-11 is already running, not starting again." >&2
	    exit 0
	fi

	echo "Starting PiDP-11 with boot number $2"
	#cd $pidp_dir
	screen -dmS pidp11 $pidp11 $2
	status=$?
	return $status
}

do_stop() {
	is_running
	if [ $? -eq 0 ]; then
	    echo "PiDP-11 is already stopped." >&2
	    status=1
	else
	    echo "Stopping PiDP-11"
	    screen -S pidp11 -X quit
	    status=$?
	    sleep 1
	    pkill client11
	    pkill server11
	    sleep 1
	    pkill client11
	    pkill server11
	fi
	return $status
}


case "$1" in
  start)
	do_start $1 $2
	;;

  stop)
	do_stop
	;;

  restart)
	do_stop
	sleep 4
	do_start $1 $2
	;;

  status)
	screen -ls pidp11 | egrep '[0-9]+\.pidp11'
	;;

  stat)
	do_stat
	;;
  ?)
	echo "Usage: pdp11control {start|stop|restart|status|stat}" || true
	exit 1
	;;
  *)
	do_stat
	if [ $status = 0 ]; then
		read -p "(S)tart, Start with boot (number), or (C)ancel? " respx
		case $respx in
			[Ss]* )
				do_start
				;;
			[0-9]* )
				#boot_number=$respx
				# convert to decimal
				#boot_number=$((8#$ooot_number))
				set -- "$1" "$respx"
				echo reassigned $2
				do_start $1 $2
				;;
			[Cc]* )
				exit 1
				;;
			* )
				echo "Please answer with S, a boot number, or C.";;
		esac
	else
		read -p "(S)top or (C)ancel? " respx
		case $respx in
			[Ss]* )
				do_stop
				;;
			[Cc]* )
				exit 1
				;;
			* )
				echo "Please answer with S or C.";;
		esac
	fi
esac
exit 0

