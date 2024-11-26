
#!/bin/bash

# stop on error
set -e

# Debugging:
# set -x

pwd
export MAKETARGETS="pidp1170_blinkenlightd"

#export MAKE_CONFIGURATION=DEBUG
export MAKE_CONFIGURATION=RELEASE

(
# the Blinkenlight API server for PiDP11
cd /opt/pidp11/src/11_pidp_server/scanswitch
make
cd ../pidp11
echo ; echo "*** blinkenlight_server for PiDP11"
MAKE_TARGET_ARCH=RPI make $MAKEOPTIONS $MAKETARGETS
)

echo
echo "Server and scansw binaries compiled OK!"
