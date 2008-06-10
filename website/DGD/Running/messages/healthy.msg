From dgd@dworkin.nl  Mon Aug  1 00:53:01 2005
From: dgd@dworkin.nl (Felix A. Croes)
Date: Sun Jul 31 23:53:01 2005
Subject: [DGD] Requirements of a healthy mudlib

=?ISO-8859-1?Q?Petter_Nystr=F6m?= wrote:

> In the past two months, my DGD-based mud has crashed twice without any 
> clue in the log. I have realized that DGD is a very robust program, but 
> that it do lean on the mudlib for some things or it will go down hard. For 
> example, I have previously had DGD crashing when there has been functions 
> missing in the driver object. But I have previously always got a helpful 
> error message in the log, which has then helped me to successfully fix up 
> my mudlib to prevent the problem from reaching the driver. This time I 
> feel I am more stuck.
>
> I am running DGD version 1.2.111 hacked to allow ansi, using the
> following command line: ./dgd-1.2.111-ansi mud.dgd 2> log.txt &

This is most likely infinite recursion in the mudlib without a limit on
ticks or stack depth.  But it might be a genuine bug in DGD.  When
running into such errors, the best thing to do is to enable coredumps
(I assume you're using linux or another unix derivate) and to examine
the core left after any crash.

This doesn't prevent crashes, but it will pinpoint the cause, either
allowing you to fix a bug in your mudlib later, or letting file a
bugreport which will allow me to fix a bug in DGD.

Enabling core dumps in linux:

    ulimit -c unlimited

before starting DGD, or within the script that (re)starts DGD.  Once
you have a core dump,

    gdb dgd-1.2.111-ansi path-to-core

and

    bt

to see the stack backtrace.

Regards,
Dworkin
