# Github repository for the PiDP-11 project

Main web sites:
- https://obsolescence.dev/pidp11 - Overview & context
- https://obsolescence.wixsite.com/obsolescence/pidp11 - Further details
- https://groups.google.com/g/pidp-11 - User group

<img src="https://obsolescence.dev/images/pidp11/pidp11.jpg" align="center" /> 

Credits: The core components in the PiDP-11 project are the simH simulator (https://github.com/open-simh/simh) and Joerg Hoppe's BlinkenBone modifications (https://www.retrocmp.com).

# Install instructions

    cd /opt
    sudo git clone https://github.com/obsolescence/pidp11.git
    /opt/pidp11/install/install.sh

Note that you do not necessarily need to have the PiDP-11 hardware. 

This will run on any Pi, but without PiDP-11 front panel hardware, you'll have to tell which OS you want to boot from the command line rather than from the front panel switches. See the manual for a how-to: https://obsolescence.dev/pidp11/PiDP-11_Manual.pdf

This will ALSO run on any Linux laptop (command line only, no desktop features, tested on Ubuntu 24.04) and presumably, Windows 11 with WSL2 subsystem (although untested). Just in case you want to have a mobile PDP-11 or want to develop on the go.

# Move to Github & the previous install package

We moved to Github only in December 2024, and this is still considered the new beta version. 

Before December 2024, the PiDP-11 install was done through downloading a tarball.<br>
You might still see references to that old version in the manual's various chapters and the PiDP-11 Google Group.<br>
The old version still works fine, and in case you have any problems, please fall back on that version (with feedback please!).<br>

This new Github version contains some meaningful improvements though:
- VT11 graphics display works under Wayland (now the default for Raspberry Pis in general and for the PiDP-11, too)
- unix v1 is added, it has been reconstructed!
- the install script install Chase Covello's updated 2.11BSD and Johnny Bilquist's updated RSX-11M+. *Importantly*, you can now rerun the install script to grab any of their updates. These are the two oldest operating systems with active maintenance :-)

# Operating the PiDP-11

Important to understand, this version has a fundamental change to the controls compared to the old version. The PiDP-11 is now managed like the PiDP-10. Which means:

**pdp11control** is the command to control the (simulated) PDP-11. From the Linux command line:
- `pdp11control stop`, `pdp11control status` do what you'd expect.<br> Please shut down PDP-11 operating systems before `pdp11control stop` (applies to unices and RSX).
- `pdp11control restart` restarts the PDP-11 (when you have a PiDP-11, it's already running when you power up though, and you'd normally do a restart using the front panel of course).
- `pdp11control start x` or `pdp11control restart x` can be used without PiDP-11 hardware. `x` is the boot number you want to run, as shown in the default Idled boot mode, see below.
- - You can also use the desktop icon for `pdp11control`. 

<br><br>
![menu](https://github.com/user-attachments/assets/b7ba9f3f-6eac-4df2-badf-35c045355a78)

**pdp11** is the simple way to get access to the PDP-11 console terminal.<br>
- The PDP-11's console terminal gets 'grabbed by' / displayed on the latest Linux terminal you've logged in to, or called `pdp11` from. Thus, you can switch from having the pdp11 terminal on the Pi's GUI, to a pdp11 terminal through a remote ssh or telnet session - you can grab it where ever you want it. Or close it. The PDP-11 does not notice, its virtual terminal (encapsulated in the linux `screen` utility) keeps running in the background.
- So `ssh pi@pidp11.local` will get you straight into the PDP-11 from your laptop. 

A nicer terminal than the basic `pdp11` is **Angelo Papenhoff's neat VT-52 simulator**, now included. Double click the desktop icon for it, use F11 to switch between full screen and windowed. See the screen shots:
<table border="1" cellpadding="10">
  <tr>
      <td></td><img src="https://github.com/user-attachments/assets/1aaf1d55-d983-4f78-9256-6cb04ae4e5d4" width="48%" align="left" />
      <td></td><img src="https://github.com/user-attachments/assets/fc0b7a3c-b8de-4968-80c4-d02a53a36e12" width="48%" align="left" />
</tr>
</table>
For either maximum pain or maximum demonstration value (it depends on your mindset), a clattering <b>Teletype Model 33 simulator</b> is also present. With sampled Teletype noise and the excruciating 10cps speed. Just close it when you've had enough. Lots more nice terminal simulations can be used to access the PiDP-11 from your laptop. See https://github.com/aap/vt05, https://github.com/larsbrinkhoff/terminal-simulator and https://github.com/rricharz/Tek4010/ for starters.
<br>
<img src="https://github.com/user-attachments/assets/31427701-1230-4438-990f-a579b4ab51e1" width="48%" align="left" />
<br>
The **hidden front panel controls** (if you have PiDP-11 front panel hardware!) are unchanged from how they were described in the manual: https://obsolescence.dev/pidp11/PiDP-11_Manual.pdf. Reboot into a new OS with the front panel switches set, or shut down with HALT enabled, pressing the top rotary knob.
