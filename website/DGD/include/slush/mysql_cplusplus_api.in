From: dgd at list.imaginary.com (Noah Gibbs)
Date: Wed Nov 20 01:34:01 2002
Subject: [DGD] Using MySQL C++ API

  Hi.  First, let me echo Keith Dunwoody's excellent sentiments.  He's
basically right.  Then let me summarize why quickly.

  DGD is really, really cool.  It does a couple of things that no other
language (except maybe the Cold server) does.  Unfortunately, those same
really cool tricks keep it from doing good callout to other, less-cool
languages.

  DGD allows atomic functions -- if DGD runs out of ticks or an error
occurs partway through, it just rolls the whole thing back, including
global variable writes, as though it never happened.  If you've been
calling out to code that doesn't do that, you can't roll it back because
you don't know all the half-assed stuff it's not finished doing yet.  If
you roll back in some obvious way and leave stuff half-done then chances
are really good that it'll crash when you next play with it.
  Believe me, I spent a long time trying to get Perl to be interruptible
in this way without constantly crashing the interpreter.  The only way I
figured out how to do it is one I'll recommend to you, but it boils down
to starting (and restarting, probably) a new server in a separate process
and never using it inside any atomic functions.  Your MySQL stuff isn't
currently atomic-able, so you can't mix it with DGD.

  DGD has exception handling that can cause similar problems, and most
speicifically the way it times out when it runs out of cycles -- again,
it's that interruptability problem.

  DGD's multithreading works the same way that its atomic functions do,
with speculative execution.  So that's right out as well.

  Essentially DGD is just really unfriendly to calling out to libraries in
other languages.  You can get around some of this by writing simple things
as chunks of DGD itself, but that doesn't necessarily mean it's trivial
(or easy, or reasonable) to do.  It also means that you now have to
distribute your own modified DGD for other people to run your
code.  That's a bad thing, especially in a community this small and
close-knit, who you'll be asking for help with debugging :-)

-- 
See my page of DGD documentation at
"http://phantasmal.sourceforge.net/DGD/"
If you post to the DGD list, you may see yourself there!
