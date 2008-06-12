From: dgd at list.imaginary.com (Felix A. Croes)
Date: Mon May  5 15:21:01 2003
Subject: [DGD] Implement Pico Editor

Lord Lerkista wrote:

> Hi, i'm trying to implement Pico as the editor for my Mud, but it have 
> a problem
> i don't know how to send the editor to the player connection, if i call 
> pico from
> the mud the editor opens in the shell where i run the Mud, anyone have 
> tried
> that?? I need help

Using the actual pico editor would require a lot of DGD hacking.  First,
you'd have to extend the subset of the telnet protocol implemented by
DGD to allow character mode and a host of other things -- probably
everything in the telnet standard.  Then, DGD must be made to execute
pico, piping input and output through the telnet front end for the
duration of the pico process.

It is certainly possible to do all of that, but it is, shall we say,
a non-trivial amount of work.

Regards,
Dworkin
