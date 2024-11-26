#!/bin/sh
sys=$1
awk '$1 == '$sys' { sys = $2; exit } END { if(sys) print sys; else print "default" }' < /opt/pidp11/systems/selections
