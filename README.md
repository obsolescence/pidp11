![image](https://obsolescence.dev/images/pidp11/pidp11.jpg)

# Github repository for the PiDP-11 project

Main web sites:

https://obsolescence.dev/pidp11 - Overview & context

https://obsolescence.wixsite.com/obsolescence/pidp11 - Further details

Credits: The core components in the PiDP-11 project are the simH simulator (https://github.com/open-simh/simh) and Joerg Hoppe's BlinkenBone modifications(https://www.retrocmp.com).

# Install instructions

    cd /opt
    sudo git clone https://github.com/obsolescence/pidp11.git
    /opt/pidp11/install/install.sh

Note that you do not necessarily need to have the PiDP-11 hardware. 

This will run on any Pi, but you have to tell it which OS you want to boot from the command ine rather than from the front panel. See the manual for a how-to: https://obsolescence.dev/pidp11/PiDP-11_Manual.pdf

This will ALSO run on any Linux laptop (tested on Ubuntu 24.04) and presumably, Windows 11 with WSL2 subsystem (although untested). Just in case you want to have a mobile PDP-11 or want to play.

# Move to Github & the previous install package

We moved to Github only in November 2024, so this is still considered a beta version. 

Before December 2024, the PiDP-11 install was done through downloading a tarball. 
You might still see references to that old install method and that old version in the manual's various chapters and the PiDP-11 Google Group. 
The old version still works fine, and in case you have any problems, please fall back on that version (with feedback please!).

This Github version of the software contains some meaningful improvements though.
- everything runs in user space
- the graphics display works under Wayland (now the default for Raspberry Pis in general and for the PiDP-11, too)
- unix v1 is added
- the install script can install Chase Covello's updated 2.11BSD and Johnny Bilquist's updated RSX-11M+. *Importantly*, you can rerun the install script for any future updates they do, without waiting for the PiDP-11 package to be updated!

# Operating the PiDP-11

This is important to understand, it is a fundamental change to the controls compared to the old version. The PiDP-11 is now managed like the PiDP-10. Which means:

**pdp11control** is the command to control the (simulated) PDP-11.<br 
- `pdp11control stop`, `pdp11control status` do what you'd expect.
- `pdp11control start` starts the PDP-11 simulation when you have a PiDP-11. 
When you just run the package on a Naked Pi or a Linux laptop, you need to do `pdp11control start x`, where `x` is the boot number you want to run:
![menu](https://github.com/user-attachments/assets/b7ba9f3f-6eac-4df2-badf-35c045355a78)

**pdp11** is the simple way to get access to the PDP-11 console terminal. 
The PDP-11's console terminal gets 'grabbed by' / displayed on the latest Linux terminal you've logged in to, or called `pdp11` from. Thus, you can switch from having the pdp11 terminal on the Pi's GUI, to a pdp11 terminal through a remote ssh or telnet session - you can grab it where ever you want it. Or close it. The PDP-11 does not notice, its virtual terminal (encapsulated in the linux `screen` utility) keeps running in the background. So `ssh pi@pidp11.local` will get you straight into the PDP-11 from your laptop. 

A nicer terminal is **Angelo Papenhoff's neat VT-52 simulator**, now included. Double click the desktop icon for it, use F11 to switch between full screen and windowed. See the screen shots:
![full-screen](https://github.com/user-attachments/assets/91929dea-749f-446c-9219-528f788ba42a)![desktop-menu](https://github.com/user-attachments/assets/14b0f3ee-dba2-4eb8-8c9b-b8c816a0f7f7)


For either maximum pain or maximum demonstration value (it depends on your mindset), a **Teletype Model 33 simulator** is also available. With sampled Teletype noise and the excruciating 10cps speed. Just close it when you've had enough.

The hidden front panel controls (if you have a PiDP-11 front panel!) are unchanged from how they were described in the manual: https://obsolescence.dev/pidp11/PiDP-11_Manual.pdf
