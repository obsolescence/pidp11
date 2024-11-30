#1/bin/sh
#
#
# install script for PiDP-11
# v20241127
#
PATH=/usr/sbin:/usr/bin:/sbin:/bin

# check this script is NOT run as root
if [ "$(whoami)" == "root" ]; then
    echo script must NOT be run as root
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
	    break;;
        [Nn]* ) 
	    echo Skipped install of dependencies - if not installed already, pidp11 will not work
            break;;
        * ) echo "Please answer Y or N.";;
    esac
done


# 20231218 - deal with user choice of precompiled 64/32 bit or compile from src
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
	    if [ "$ARCH" == "arm64" ]; then
                subdir=backup64bit-binaries
	        echo "This Raspberry Pi is running a 64-bit operating system."
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
	    echo 
	    echo Copied precompiled binaries into place.
            break;;
        [Cc]* ) 
            sudo rm $pidpath/src/02.3_simh/4.x+realcons/bin-rpi/pdp11_realcons
            sudo rm $pidpath/src/11_pidp_server/scanswitch/scansw
            sudo rm $pidpath/src/11_pidp_server/pidp11/bin-rpi/pidp1170_blinkenlightd
            sudo $pidpath/src/makeclient.sh
            sudo $pidpath/src/makeserver.sh
	    echo
            echo recompiled PiDP-11 binaries from source.
	    break;;
        [Ss]* ) 
            echo Skipped putting new binaries in place, things left untouched. 
	    echo Rerun install if PiDP-11 does not work!
            break;;
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
		# Run xhost + at GUI start to allow access for vt11. 
		# Proof entire setup needs redoing.
		# (this will not work on Wayland, just X11)
		echo
		echo
		echo NOTE: if you want to use RT-11 VT graphics, then:
		echo make sure to run sudo raspi-config, and enable X11 instead of Wayland.
		echo ...details: in raspi-config, choose Advanced Options-Wayland-X11
		echo
		echo Alternatively, RT-11 graphics under Wayland require you to restart the PDP-11
		echo simulator manually with pidp11.sh in the pidp11 bin directory
		echo
		echo In a hurry? Leave this for later, not critical.
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
		    echo "OK, Line already exists in $config_file"
		fi


		# Set up pidp11 init script
		if [ ! -x /opt/pidp11/etc/rc.pidp11 ]; then
			echo pidp11 not found in /opt/pidp11. Abort.
			exit 1
		else
			sudo ln -s /opt/pidp11/etc/rc.pidp11 /etc/init.d/pidp11
			sudo update-rc.d pidp11 defaults
			echo pidp11 added to init.d
		fi


		# setup 'pdp.sh' (script to return to screen with pidp11) 
		# in home directory if it is not there yet
		test ! -L /home/pi/pdp.sh && ln -s /opt/pidp11/etc/pdp.sh /home/pi/pdp.sh
		# easier to use - just put a pdp11 command into /usr/local
		sudo ln -f -s /opt/pidp11/etc/pdp.sh /usr/local/bin/pdp11

		# add pdp.sh to the end of pi's .profile to let a new login 
		# grab the terminal automatically
		#   first, make backup .foo copy...
		test ! -f /home/pi/profile.foo && cp -p /home/pi/.profile /home/pi/profile.foo
		#   add the line to .profile if not there yet
		if grep -xq "/home/pi/pdp.sh" /home/pi/.profile
		then
			echo .profile already done, OK.
		else
			sed -e "\$a/home/pi/pdp.sh" -i /home/pi/.profile
		fi

	    break;;
        [Nn]* ) 
	    echo Skipped software install
            break;;
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
            sudo wget -O /opt/pidp11/systems.tar.gz http://pidp.net/pidp11/systems.tar.gz
            sudo gzip -d systems.tar.gz
            sudo tar -xvf systems.tar
	    break;;
        [Nn]* ) 
	    echo PDP-11 operating systems not added at your request. You can do it later.
            break;;
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
	    break;;

	[Nn]* ) 
	    echo Skipped. You can do it later by re-running this install script.
            break;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo
echo Desktop update done.




# 20241126 Add Chase Covello's updated 2.11BSD straight from his github
# =============================================================================
while true; do
    echo
    read -p "2024 update: Add Chase Covello's updated 2.11BSD ? " prxn
    case $prxn in
        [Yy]* ) 



		# Directory path
		dir="/opt/pidp11/systems/211bsd+"

		echo "Checking if xz-utils is installed for decompression:"
		sudo apt install xz-utils

		# Check if the directory for Chase Covello's 211BSD already exists
		if [ -d "$dir" ]; then
		    echo "You already have the 211BSD+ directory!"
		    echo "boot.ini and the disk image in $dir will be updated."
		else
		    echo
		    echo "Creating $dir..."
		    sudo mkdir "$dir"
		    echo
		fi

	        echo "Downloading from github.com/chasecovello/211bsd-pidp11"
	        echo "please visit that page for more information"
	        echo
	        sudo wget -O "${dir}/boot.ini" https://raw.githubusercontent.com/chasecovello/211bsd-pidp11/refs/heads/master/boot.ini 
		sudo wget -O "${dir}/2.11BSD_rq.dsk.xz" https://github.com/chasecovello/211bsd-pidp11/raw/refs/heads/master/2.11BSD_rq.dsk.xz
		echo
		echo Decompressing...
		echo
		cd "${dir}"
		sudo unxz -f ./2.11BSD_rq.dsk.xz
		echo
		echo Modifying boot.ini by commenting out the icr device for bmp280 i2c
		echo
	        sudo sed -i 's/^attach icr icr.txt$/;attach icr icr.txt/' "${dir}/boot.ini"
		echo
		echo Modifying boot.ini by enabling the line set realcons connected:
                sudo sed -i 's/^;set realcons connected$/set realcons connected/' "${dir}/boot.ini"
		echo
		echo ...Done. Set SR switches to octal 0211 to boot into this newly installed 211.BSD
		echo    Do not forget to visit github.com/chasecovello/211bsd-pidp11 
		echo    to find out about all the good stuff on this update!
		echo
		echo

		# Insert a new line for the OS in the boot options selections file
		file="/opt/pidp11/systems/selections"
		new_line="0211"$'\t'"211bsd+"
		echo
		echo
		# Check if the file exists, create it if it doesn't
		if [ ! -f "$file" ]; then
		    echo "$file does not exist. That's fatal - check your /opt/pidp11 directory..."
		    exit 1
		fi

		# Add the new line to selections and sort it alphabetically
		echo "...Adding boot option $new_line to selections menu"
		#sudo sh -c 'cat "$file" | sort | uniq > temp_file && mv temp_file "$file"'
		#sudo sh -c "{ cat \"$file\"; echo \"$new_line\" } | sort | uniq > temp_file && mv temp_file \"$file\""
		sudo sh -c "{ cat \"$file\"; echo \"$new_line\"; } | sort | uniq > temp_file && mv temp_file \"$file\""

		echo "Line added. Reboot with SR switches set to 0211 to boot the new system."



	    break;;

	[Nn]* ) 
	    echo Skipped. You can do it later by re-running this install script.
            break;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo
echo Done. Please do a sudo reboot and the front panel will come to life.






# 20241126 Add Johnny Bilquist's latest RSX-11MPlus with BQTC/IP
# =============================================================================
while true; do
    echo
    read -p "2024: Add Johnny Bilquist's latest RSX-11MPlus with BQTC/IP? " prxn
    case $prxn in
        [Yy]* ) 



		# Directory path
		dir="/opt/pidp11/systems/rsx11bq"
		# Check if the directory for Johnny Bilquists RSX-11 already exists
		if [ -d "$dir" ]; then
		    echo "You already have the Bilquist RSX11BQ directory!"
		    echo "Only the disk image in $dir will be updated."
		else
		    echo
		    echo "Creating $dir..."
		    sudo mkdir "$dir"
		    echo
		    echo "Copying boot.ini from install/boot.ini.bilquist directory..."
		    sudo cp /opt/pidp11/install/boot.ini.bilquist "${dir}/boot.ini"
		fi

		echo
		echo "Getting files from ftp://ftp.dfupdate.se/pub/pdp11/rsx/pidp/"
		echo "please visit http://mim.stupi.net/pidp.htm for more information"
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
		    sudo wget --user="anonymous" --password="$email" -O ${file} "${ftp_url}/${file}"
		    echo
		    echo Decompressing...
		    echo
		    sudo gunzip -f "${file}" 
		done

		echo
		echo
		echo ...Done. Set SR switches to octal 2024 to boot this newly installed RSX.
		echo    Do not forget to visit http://mim.stupi.net/pidp.htm 
		echo    to find out about all the good stuff in this update!
		echo
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
		sudo sh -c "{ cat \"$file\"; echo \"$new_line\"; } | sort | uniq > temp_file && mv temp_file \"$file\""

		echo "Line added. Reboot with SR switches set to 2024 to boot the new system."



	    break;;

	[Nn]* ) 
	    echo Skipped. You can do it later by re-running this install script.
            break;;
        * ) echo "Please answer Y or N.";;
    esac
done
echo
echo Done. Please do a sudo reboot and the front panel will come to life.




