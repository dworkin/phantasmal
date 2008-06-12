From: Par Winzell
Date: Mon, 25 Jun 2001 08:21:01 -0700
Subject: Re: [DGD]CPU usage

David,

 > Our mud is consistently using 30-60%, and sometimes jumping to 70% of
 > a celeron 733 I think, it has 256 megs of ram also. The number of people
 > playing is usually around 10-17 and most of these are chatting, with 2-3
 > wizards who are coding. Does anybody have any suggestions on bringing
 > down our CPU usage? I have been warned by our host that our usage has 
 > become excessive. Thanks.

DGD just runs LPC code. Clearly your LPC code tries to do more than
your machine can handle. Some suggestions for problem analysis,

 *) First, chart how CPU usage varies with users. If it's at 50% with
    no users on, then clearly background work is the culprit. If it's
    at 10% with no users on but 70% in the evening, then code executed
    as a direct result of player commands would be the place to look.

 *) If it's high in CPU, it's probably not limited by I/O or memory,
    which is a good thing. It suggests the slowness is due to running
    LPC code rather than, say, too low a swap fragment or worse, the
    machine swapping at the OS level.

 *) Technically less than completely trivial but very wortwhile is to
    get profiling data. For DGD this is currently done by precompiling
    the whole library (is there documentation on this process)? If you
    go this route, I heartily suggest using Dark's/Nino's EPP 2.1.

    The precompiler turns LPC into C code. Then, you compile it all in
    a profiling compiler (which measures how much time is spent in what
    functions) and analyse the result. Non-trivial but often useful.

    I've made stabs at an internal DGD profiler, but the version I put
    together is exceedingly limited. I may revisit this problem though
    as Skotos' games get larger and (even) more difficult to analyze.

 *) Enable and disable suspicious parts of the code. For example, if you
    have complex code that is run every time somebody moves through a
    room, try temporarily commenting out that code and see what happens
    to the load. You can do things like move east and then west 10,000
    times and see how long it takes on a totally unloaded (test) Mud.

 *) DGD has a tick counter. All LPC that is executed uses up ticks and
    you can limit that amount of ticks any thread had. You could slowly
    lower this limit until you start seeing 'out of ticks' errors in
    the driver log and stare carefully at the stack trace to figure out
    where in the mudlib that tends to occur.


Just a few common-sense suggestions.

Zell
