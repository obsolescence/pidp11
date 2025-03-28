#!/bin/bash
#
#
# install script for PiDP-11
# v20241127
#
#PATH=/usr/sbin:/usr/bin:/sbin:/bin


# check this script is NOT run as root
if [ "$(whoami)" = "root" ]; then
    echo script must NOT be run as root
    exit 1
fi

if [ ! -d "/opt/pidp11" ]; then
    echo clone git repo into /opt/
    exit 1
fi

echo
echo
echo PiDP-11 install script
echo ======================
echo
echo The script can be re-run at any time to change things. Re-running the install
echo script and answering \'n\' to questions will leave those things unchanged.
echo You can recompile from source, but it is easier to just install the precompiled
echo binaries. 
echo
echo Too Long, Didn\'t Read?
echo Just say Yes to everything.
echo
echo


# Set required access privileges to pidp11 simulator
# =============================================================================

while true; do
    echo
    read -p "Set required access privileges to pidp11 simulator? " yn
    case $yn in
        [Yy]* )
            # make sure that the directory does not have root ownership
            # (in case the user did a simple git clone instead of 
            #  sudo -u pi git clone...)
            myusername=$(whoami)
            mygroup=$(id -g -n)
            sudo chown -R $myusername:$mygroup /opt/pidp11
            # make sure pidp11 simulator has the right privileges
            # to access GPIO with root privileges:
            sudo chmod +s /opt/pidp11/src/11_pidp_server/scanswitch/scansw
            sudo chmod +s /opt/pidp11/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            # to run a RT thread:
            sudo setcap cap_sys_nice+ep /opt/pidp11/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
	    echo Done.
	    break
            ;;
        [Nn]* ) 
            echo Skipped the setting of access privileges.
	    break
            ;;
        * ) echo "Please answer yes or no.";;
    esac
done

# Install required dependencies
# =============================================================================
while true; do
    echo
    read -p "Install the required dependencies? " prxn
    case $prxn in
        [Yy]* ) 
            sudo apt update
            #Install SDL2, optionally used for PDP-11 graphics terminal emulation
            sudo apt install -y libsdl2-dev
            #Install pcap, optionally used when PDP-11 networking is enabled
            sudo apt install -y libpcap-dev
            #Install readline, used for command-line editing in simh
            sudo apt install -y libreadline-dev
            # Install screen
            sudo apt install -y screen
            # Install newer RPC system
            sudo apt install -y libtirpc-dev
            break
	    ;;
        [Nn]* ) 
            echo Skipped install of dependencies - OK if installed already
            break
	    ;;
        * ) echo "Please answer Y or N.";;
    esac
done

# Configure PCAP interface so it can be used without sudo

echo
read -p "Configure PCAP permissions for current user? " prxn
case $prxn in
    [Yy]* ) 
	    # Set permissions for client11 to be able to access the lobpcap interface in simh without sudo
	    sudo setcap cap_net_raw,cap_net_admin=eip /opt/pidp11/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
	    ;;
    [Nn]* ) 
	    echo Skipped setting client11 permissions - OK if already set, otherwise Ethernet will not work
            echo To set this at a later point in time run this command 'sudo setcap cap_net_raw,cap_net_admin=eip /opt/pidp11/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons'
	    ;;
	* ) echo "Please answer Y or N.";;
esac


# Deal with user choice of precompiled 64/32 bit or compile from src
# =============================================================================
pidpath=/opt/pidp11

while true; do
    echo
    read -p "(Y) to install precompiled binaries, or (C)ompile from source, or (S)kip? " prxn
    case $prxn in
        [Yy]* ) 
            # Query the system architecture
            ARCH=$(dpkg-architecture --query DEB_HOST_ARCH)
            echo
            if [ "$ARCH" = "arm64" ]; then
                    subdir=backup64bit-binaries
                echo "This Raspberry Pi is running a 64-bit operating system."
            elif [ "$ARCH" = "amd64" ]; then
                    subdir=backupAmd64-binaries
                echo "This is a amd64 Linux system, not a Raspberry Pi - OK, installing."
                else
                    subdir=backup32bit-binaries
                echo "This Raspberry Pi is running a 32-bit operating system."
            fi
            echo
            echo Copying binaries from /opt/pidp11/bin/$subdir
            sudo cp $pidpath/bin/$subdir/pdp11_realcons $pidpath/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
            sudo cp $pidpath/bin/$subdir/scansw $pidpath/src/11_pidp_server/scanswitch/scansw
            sudo cp $pidpath/bin/$subdir/pidp1170_blinkenlightd $pidpath/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            sudo cp $pidpath/bin/$subdir/vt52 $pidpath/bin/
            sudo cp $pidpath/bin/$subdir/sty $pidpath/bin/
            sudo cp $pidpath/bin/$subdir/tek4010 $pidpath/bin/
            # to run a RT thread:
            sudo setcap cap_sys_nice+ep /opt/pidp11/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            echo 
            echo Copied precompiled binaries into place.
            break
	    ;;
        [Cc]* ) 
            sudo rm $pidpath/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
            sudo rm $pidpath/src/11_pidp_server/scanswitch/scansw
            sudo rm $pidpath/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            sudo $pidpath/src/makeclient.sh
            sudo $pidpath/src/makeserver.sh
            echo
            echo recompiled PiDP-11 binaries from source.
            break
	    ;;
        [Ss]* ) 
            echo Skipped putting new binaries in place, things left untouched. 
            echo Rerun install if PiDP-11 does not work!
            break
	    ;;
        * ) echo "Please answer Y, C, or S.";;
    esac
done


# Install the pidp11 software
# =============================================================================
while true; do
    echo
    read -p "Install PiDP-11 package into OS? " prxn
    case $prxn in
        [Yy]* ) 
            # setup 'pdp.sh' (script to return to screen with pidp11) 
            # in home directory if it is not there yet
            test ! -L /home/pi/pdp.sh && ln -s /opt/pidp11/etc/pdp.sh /home/pi/pdp.sh
            # easier to use - just put a pdp11 command into /usr/local
            sudo ln -f -s /opt/pidp11/etc/pdp.sh /usr/local/bin/pdp11
            # the pdp11control script into /usr/local:
            sudo ln -f -s /opt/pidp11/bin/pdp11control.sh /usr/local/bin/pdp11control

            if [ "$ARCH" = "amd64" ]; then
                echo skipping autostart and rpcbind, because this is not a Raspberry Pi
                echo Not a problem: start manually by typing 
                echo pdp11control start x
                echo ...where x is the OS number normally set on the front panel.
                echo 
                echo Access the PDP-11 terminal by typing pdp11 afterwards.
                echo
                echo "But that is all in the manual..."
                echo
                echo copying modified pidp11.sh for Amd64...
                cp /opt/pidp11/bin/backupAmd64-binaries/pidp11.sh /opt/pidp11/bin/pidp11.sh
            else
                # enable rpcbind
                sudo systemctl enable rpcbind
                sudo systemctl start rpcbind
                #echo please check that rpcbind is up:
                #sudo systemctl status rpcbind
                echo
                echo
                echo "Autostart the PDP-11 using the GUI(Y) or .profile (H)?"
                    read -p "-- Y recommended, H is for headless Pis without GUI:" yhn
                case $yhn in
                      [Yy]* ) 
                        mkdir -p ~/.config/autostart
                        cp /opt/pidp11/install/pdp11startup.desktop ~/.config/autostart
			echo
			echo Autostart via .desktop file for GUI setup
                        break
			;;
                      [NnHh]* ) 
                        # add pdp11 to the end of pi's .profile to let a new login 
                        # grab the terminal automatically
                        #   first, make backup .foo copy...
                        test ! -f /home/pi/profile.foo && cp -p /home/pi/.profile /home/pi/profile.foo
                        #   add the line to .profile if not there yet
                        if grep -xq "pdp11 # autostart" /home/pi/.profile
                        then
                            echo .profile already contains pdp11 for autostart, OK.
                        else
                            sed -e "\$apdp11 # autostart" -i /home/pi/.profile
                        fi
			echo
			echo autostart via .profile for headless use without GUI
                        break
			;;
                      * ) echo "Please answer Y or N.";;
                    esac
            fi
            break
	    ;;
        [Nn]* ) 
            echo Skipped software install
            break
	    ;;
        * ) echo "Please answer Y or N.";;
    esac
done


# 20231218 - install all operating systems, if desired
# =============================================================================
while true; do
    echo
    read -p "Download and install the PDP-11 operating systems? " prxn
    case $prxn in
        [Yy]* ) 
            cd /opt/pidp11
            wget -O /opt/pidp11/systems.tar.gz http://pidp.net/pidp11/systems24.tar.gz
            echo "Decompressing... (might take a while)"
            gzip -d systems.tar.gz
            tar -xvf systems.tar
            break
	    ;;
        [Nn]* ) 
            echo PDP-11 operating systems not added at your request. You can do it later.
            break
	    ;;
        * ) echo "Please answer Y or N.";;
    esac
done


# 20241126 Add VT52 desktop icon
# =============================================================================
while true; do
    echo
    read -p "Add VT-52 desktop icon and desktop settings? " prxn
    case $prxn in
        [Yy]* ) 
            cp /opt/pidp11/install/vt52.desktop /home/pi/Desktop/
            cp /opt/pidp11/install/vt52fullscreen.desktop /home/pi/Desktop/
            cp /opt/pidp11/install/tty.desktop /home/pi/Desktop/
            cp /opt/pidp11/install/tek.desktop /home/pi/Desktop/
            cp /opt/pidp11/install/pdp11control.desktop /home/pi/Desktop/

            #make pcmanf run on double click, change its config file
            config_file="/home/pi/.config/libfm/libfm.conf"
            # Create the directory if it doesn't exist
            mkdir -p "$(dirname "$config_file")"
            # Add or update the quick_exec setting
            if grep -q "^\s*quick_exec=" "$config_file" 2>/dev/null; then
                echo ...Update existing setting...
                sed -i 's/^\s*quick_exec=.*/quick_exec=1/' "$config_file"
            else
                echo ...Adding the config file, it does not exist yet
                echo -e "[config]\nquick_exec=1" >> "$config_file"
            fi
        
            # wallpaper
            echo $XDG_RUNTIME_DIR
            echo ==========================
            pcmanfm --set-wallpaper /opt/pidp11/install/wallpaper.jpeg --wallpaper-mode=fit

            echo
            echo "Installing Teletype font..."
            echo
            mkdir ~/.fonts
                cp /opt/pidp11/install/TTY33MAlc-Book.ttf ~/.fonts/
            fc-cache -v -f


            echo "Desktop updated."
            break
	    ;;

        [Nn]* ) 
            echo Skipped. You can do it later by re-running this install script.
            break
	    ;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo



# 20241126 Add Chase Covello's updated 2.11BSD straight from his github
# =============================================================================
while true; do
    echo
    read -p "2024 update: Add Chase Covello's updated 2.11BSD ? " prxn
    case $prxn in
        [Yy]* ) 
            # Directory path
            dir="/opt/pidp11/systems/211bsd+"
            echo
            echo "Checking if xz-utils is installed for decompression:"
            sudo apt install xz-utils

            # Check if the directory for Chase Covello's 211BSD already exists
            if [ -d "$dir" ]; then
                echo
		echo "You already have the 211BSD+ directory!"
                echo "boot.ini and the disk image in $dir will be updated."
            else
                echo
                echo "Creating $dir..."
                mkdir "$dir"
                echo
            fi
            echo
            echo "Downloading from github.com/chasecovello/211bsd-pidp11"
            echo "please visit that page for more information"
            echo
            # --no-check-certificate because of unclear Encryption errors from github
            curl -L -o "${dir}/boot.ini" \
            "https://raw.githubusercontent.com/chasecovello/211bsd-pidp11/refs/heads/master/boot.ini" 
            curl -L -o "${dir}/2.11BSD_rq.dsk.xz" \
            "https://github.com/chasecovello/211bsd-pidp11/raw/refs/heads/master/2.11BSD_rq.dsk.xz"
            echo
            echo Decompressing...
            cd "${dir}"
            unxz -f ./2.11BSD_rq.dsk.xz
            echo
            echo Modifying boot.ini by commenting out the icr device for bmp280 i2c
            sed -i 's/^attach icr icr.txt$/;attach icr icr.txt/' "${dir}/boot.ini"
            echo Modifying boot.ini by enabling the line set realcons connected:
            sed -i 's/^;set realcons connected$/set realcons connected/' "${dir}/boot.ini"
            echo
            echo ...Done.

            # Insert a new entry for the OS in the boot options selections file
            file="/opt/pidp11/systems/selections"
            new_line="0211"$'\t'"211bsd+"
            echo
            # Check if the file exists, create it if it doesn't
            if [ ! -f "$file" ]; then
                echo "$file does not exist. That's fatal - check your /opt/pidp11 directory..."
                exit 1
            fi

            # Add the new line to selections and sort it alphabetically
            echo "...Adding boot option $new_line to selections menu"
            sh -c "{ cat \"$file\"; echo \"$new_line\"; } | sort | uniq > temp_file && mv temp_file \"$file\""

            echo ...Done. 
	    echo
	    echo Reboot with SR switches set to 0211 to boot the new system.
            echo Do not forget to visit github.com/chasecovello/211bsd-pidp11 
            echo to find out about all the good stuff on this update!
            echo
            break
	    ;;

        [Nn]* ) 
            echo Skipped. You can do it later by re-running this install script.
            break
	    ;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo


# 20241126 Add Johnny Billquist's latest RSX-11MPlus with BQTC/IP
# =============================================================================
while true; do
    echo
    read -p "2024: Add Johnny Billquist's latest RSX-11MPlus with BQTC/IP? " prxn
    case $prxn in
        [Yy]* ) 
            # Directory path
            dir="/opt/pidp11/systems/rsx11bq"
            # Check if the directory for Johnny Billquists RSX-11 already exists
            if [ -d "$dir" ]; then
                echo "You already have the Billquist RSX11BQ directory!"
                echo "Only the disk image in $dir will be updated."
            else
                echo
                echo "Creating $dir..."
                mkdir "$dir"
                echo
                echo "Copying boot.ini from install/boot.ini.bilquist directory..."
                cp /opt/pidp11/install/boot.ini.bilquist "${dir}/boot.ini"
            fi

            echo
            echo "Getting files from ftp://ftp.dfupdate.se/pub/pdp11/rsx/pidp/"
            echo
            echo "Files will be downloaded by anonymous ftp from dfupdate.se."
            echo "As a courtesy, leave your email address (that is ftp etiquette)"
            echo
            read -p "Enter your email address: " email
            ftp_url="ftp://ftp.dfupdate.se/pub/pdp11/rsx/pidp"
            files=("pidp.dsk.gz" "pidp.tap.gz")
            cd ${dir}
            for file in "${files[@]}"; do
                echo
                echo "Downloading $file..."
                wget --user="anonymous" --password="$email" -O ${file} "${ftp_url}/${file}"
                echo
                echo Decompressing...
                echo
                gunzip -f "${file}" 
            done

            echo
            # Insert a new line for the OS in the boot options selections file
            file="/opt/pidp11/systems/selections"
            new_line="2024"$'\t'"rsx11bq"
            echo
            echo
            # Check if the selections file exists
            if [ ! -f "$file" ]; then
                echo "$file does not exist. That's fatal - check your /opt/pidp11 directory..."
                exit 1
            fi

            # Add the new line to selections and sort it alphabetically
            echo "...Adding boot option $new_line to selections menu"
            sh -c "{ cat \"$file\"; echo \"$new_line\"; } | sort | uniq > temp_file && mv temp_file \"$file\""

            echo Done. Set SR switches to octal 2024 to boot this newly installed RSX.
            echo    Do not forget to visit http://mim.stupi.net/pidp.htm 
            echo    to find out about all the good stuff in this update!
            echo

            break
	    ;;

        [Nn]* ) 
            echo Skipped. You can do it later by re-running this install script.
            break
	    ;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo
echo Done. Please do a sudo reboot and the front panel will come to life.
echo Rerun this script if you want to do any install modifications.

