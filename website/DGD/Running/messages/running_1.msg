From: dgd at list.imaginary.com (Alex Swavely)
Date: Thu Apr  3 16:03:00 2003
Subject: [DGD] Running a DGD mud on Linux

On Thu, 3 Apr 2003, Jay Shaffstall wrote:

> I'm about ready to start a 24/7 beta test of my DGD mudlib, and have found
> a Linux machine to run it on.  This is, however, my first time at running a
> mud under Linux.  Can anyone suggest any places to look for more
> information on what all needs to be done?  I'm imagining I'll need to run a
> process periodically to check on the health of the mud server, but that's
> about as far as my imaginings go.

Gurba uses a keepalive script that periodically (run from cron) looks for
a listener on the port you specify, and if not found, runs the startup
script.

Since permission was given in the file for it to be redistributed freely,
I've included it below.

-----

#!/bin/bash
# A bash script to keep the mud up by looking at ps and seeing if
# the driver or startmud script is going, and if it isn't, then
# it invokes that command.  This is meant to be called from your
# machines cron system, so you should add an entry to your crontab
# 0,30 * * * * ~/bin/keepup
# which will check every 30 minutes to see if your mud is still up.
# grep is probably not the best way to check, a regexp is probably
# better, but I figure your executable name should be long and fairly
# unique if you invoke with the full path name, and I'd prefer this to be
# easily portable between shells, and rexexp syntax differs between
# shells.

#Redistribute freely, as usual

#
# Modify these as necessary
#
dgddir=/home/cohort/gurba
port=3000

#######
netstat="netstat -a -t -n"
searchfor="$port.*LISTEN"
startmud=$dgddir/startmud

isup=$($netstat | grep $searchfor)

if [ ${#isup} -eq 0 ]; then
  $startmud &
fi
