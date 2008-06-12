From: dgd at list.imaginary.com (Felix A. Croes)
Date: Tue Jun  3 05:31:00 2003
Subject: [DGD] Damage Weapons and Stuff (Math and Code)

Some notes on the relative speed of various functions.

The random() kfun uses either the BSD Unix C library function random(),
or the SYSV Unix C library function rand48().  I do not know which is
faster.  The randomness of either is considered to be inadequate by
many specialists, who like custom random functions for different
applications.  I expect that it is possible to implement a fast
pseudo-random number generator in LPC.  Also note that unless you know
the order in which users send their input to the server, you'll have
an extra source of randomness there which affects the order in which
calls to your pseudo-random number generator are made.

All LPC floating point numbers are currently simulated without using
floats in C.  This is of course slower than using floats, but not as
much slower as you would expect.  Also take into account that DGD
floats do not exactly match IEEE floats or doubles; they have 11
bits in the exponent and 36 in the mantissa.  A future version of
DGD is likely to use native floats.

Many of those who worry about the speed of LPC start with adding
new kfuns to DGD first.  This is doing things backwards.  LPC is
a decent programming language, so first implement your mudlib in
LPC, then optimize, then precompile, and <then> rewrite parts of
your code in C.  Chances are that the code you eventually decide
to write a kfun for, if any, is completely different from the
code that you originally had doubts about.

Finally, on how to time the performance of a function in LPC:
either go by ticks measurement, or let it do a few hundred
thousand or million iterations so that the time spent can be
measured in seconds, and then observe the actual time spent on
an unloaded system.

Regards,
Dworkin
