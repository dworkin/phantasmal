From: dgd at list.imaginary.com (Noah Lee Gibbs)
Date: Thu Apr  3 17:04:00 2003
Subject: [DGD] Running a DGD mud on Linux

On Thu, 3 Apr 2003, Alex Swavely wrote:
> Gurba uses a keepalive script that periodically (run from cron) looks for
> a listener on the port you specify, and if not found, runs the startup
> script.

  One thing to be careful of here -- you might want to check "ps" for a 
known process number as well.  Otherwise if a MUD hangs but doesn't die 
(and won't accept connections) you'll run DGD every time cron checks, and 
DGD will promptly tell you it can't use that port because somebody else 
already is.
  So having a process number, checking to see if it's still there, and 
(possibly) deciding to kill it if you can't connect after awhile seems 
like a good thing.
  Especially since (IME) DGD and your MUDLib are much more likely to get 
into an unusable state than to outright crash and stop.

-- 
See my page of DGD documentation at
"http://phantasmal.sourceforge.net/DGD/"
If you post to the DGD list, you may see yourself there!
