#1/bin/sh
#
#
# install script for PiDP-11
# v20231218
#
PATH=/usr/sbin:/usr/bin:/sbin:/bin

apt-get update
#Install SDL2, optionally used for PDP-11 graphics terminal emulation
apt-get install libsdl2-dev
#Install pcap, optionally used when PDP-11 networking is enabled
apt-get install libpcap-dev
#Install readline, used for command-line editing in simh
apt-get install libreadline-dev
# Install screen
apt-get install screen
# Install newer RPC system
apt-get install libtirpc-dev


# 20231218 - deal with user choice of precompiled 64/32 bit or compile from src
# =============================================================================
pidpath=/opt/pidp11

while true; do

    read -p "Install precompiled (3)2 bit, precompiled (6)4 bit binaries or (C)ompile from source? " prxn
    case $prxn in
        [3]* ) 
            subdir=backup32bit-binaries
            echo Copying binaries from /opt/pidp11/bin/$subdir
            sudo cp $pidpath/bin/$subdir/pdp11_realcons $pidpath/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
            sudo cp $pidpath/bin/$subdir/scansw $pidpath/src/11_pidp_server/scanswitch/scansw
            sudo cp $pidpath/bin/$subdir/pidp1170_blinkenlightd $pidpath/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            sudo cp $pidpath/bin/$subdir/vt52 $pidpath/bin/
            break;;
        [6]* ) 
            subdir=backup64bit-binaries
            sudo cp $pidpath/bin/$subdir/pdp11_realcons $pidpath/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
            sudo cp $pidpath/bin/$subdir/scansw $pidpath/src/11_pidp_server/scanswitch/scansw
            sudo cp $pidpath/bin/$subdir/pidp1170_blinkenlightd $pidpath/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            sudo cp $pidpath/bin/$subdir/vt52 $pidpath/bin/
            break;;
        [Cc]* ) 
            sudo rm $pidpath/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
            sudo rm $pidpath/src/11_pidp_server/scanswitch/scansw
            sudo rm $pidpath/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            sudo $pidpath/src/makeclient.sh
            sudo $pidpath/src/makeserver.sh
            break;;
        * ) echo "Please answer 3,6 or C.";;
    esac
done
echo $prxn - Done.

# =============================================================================


# Run xhost + at GUI start to allow access for vt11. Proof entire setup needs redoing.
# (this will not work on Wayland, just X11)
echo
echo
echo NOTE: if you want to use RT-11 VT graphics, then:
echo make sure to run 'sudo raspi-config', and enable X11 instead of Wayland.
echo ...details: in raspi-config, choose Advanced Options->Wayland->X11, then Finish to reboot
echo
echo
new_config_line="xhost +"
config_file="/etc/xdg/lxsession/LXDE-pi/autostart"
# Check if the line already exists in the config file
if ! grep -qF "$new_config_line" "$config_file"; then
    # If the line doesn't exist, append it to the file
    sudo echo "$new_config_line" >> "$config_file"
    echo "Line added to $config_file"
else
    echo "Line already exists in $config_file"
fi


# Set up pidp11 init script, provided pidp11 is installed in /opt/pidp11
if [ ! -x /opt/pidp11/etc/rc.pidp11 ]; then
	echo pidp11 not found in /opt/pidp11. Abort.
	exit 1
else
	ln -s /opt/pidp11/etc/rc.pidp11 /etc/init.d/pidp11
	update-rc.d pidp11 defaults
fi


# setup 'pdp.sh' (script to return to screen with pidp11) in home directory if it is not there yet
test ! -L /home/pi/pdp.sh && ln -s /opt/pidp11/etc/pdp.sh /home/pi/pdp.sh


# add pdp.sh to the end of pi's .profile
#   first, make backup .foo copy...
test ! -f /home/pi/profile.foo && cp -p /home/pi/.profile /home/pi/profile.foo
#   add the line to .profile if not there yet
if grep -xq "/home/pi/pdp.sh" /home/pi/.profile
then
	echo .profile already done, OK.
else
	sed -e "\$a/home/pi/pdp.sh" -i /home/pi/.profile
fi


# 20231218 - install all operating systems, if desired
# =============================================================================

while true; do
    read -p "Download and install operating systems? " prxn
    case $prxn in
        [Yy]* ) 
	    cd /opt/pidp11
            sudo wget -O http://pidp.net/pidp11/systems.tar.gz
            sudo gzip -d systems.tar.gz
            sudo tar -xvf systems.tar
	    break;;
        [Nn]* ) 
	    echo operating systems not added at your request. You can do it later.
            break;;
        * ) echo "Please answer Y or N.";;
    esac
done

# 20241126 Add VT52 desktop icon
# =============================================================================

while true; do
    read -p "Add VT-52 desktop icon? " prxn
    case $prxn in
        [Yy]* ) 
	    cp /opt/pidp11/install/vt52.desktop /home/pi/Desktop/

	    #make pcmanf run on double click, change its config file
            config_file="/home/pi/.config/libfm/libfm.conf"
            # Create the directory if it doesn't exist
            #mkdir -p "$(dirname "$config_file")"
            # Add or update the execution policy setting
            if grep -q "^\s*quick_exec=" "$config_file" 2>/dev/null; then
                # Update existing setting
                sed -i 's/^\s*quick_exec=.*/quick_exec=1/' "$config_file"
            else
                # Add the setting if it doesn't exist
                echo -e "[config]\nquick_exec=1" >> "$config_file"
            fi
            echo "Desktop updated."
	    break;;

	[Nn]* ) 
	    echo Skipped. You can do it later by re-running this install script.
            break;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo $prxn - Done.

