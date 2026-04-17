#!/bin/sh

# Helper script to handle the PiDP-11 Data Selector switch press.
# Search for a "data_switch.sh" script in the currently selected system's directory (e.g. /opt/pidp11/systems/idled/data_switch.sh)
# and exit with its return value, if it exists.  Otherwise toggle the Panel Lock functionality.

CWD_FILE=/run/pidp11/cwd
if [ -f "$CWD_FILE" ]; then
    DATA_SCRIPT="`cat $CWD_FILE`/data_switch.sh"
    if [ -f "$DATA_SCRIPT" ]; then
        . $DATA_SCRIPT
	exit $?
    fi
fi



# If no data_switch.sh script was found then exit with code 1 to toggle the panel lock
exit 1
