From: dgd at list.imaginary.com (Michael McKiel)
Date: Fri Mar 19 14:50:01 2004
Subject: [DGD] DGD crashing under Cygwin

For anyone that might run into almost instantaneous odd crashing behaviour of
the driver under Cygwin, when it previously had been fine - and want to blame
it on Windows ;) Check for circular function calls.

I wanted to test that receive_message was working for mobs in thing.c before
I had mobs, so I changed the user's receive_message() which had been working
fine with its ::receive_message(str); bit, to action(str); which is a
function in the inheritable thing.c that just calls into receive_message() in
thing.c

But since its inherited into the user object, it was just calling the user's
receive_message() which called action() which called...hehe

The only way to test, wound up temporarily making receive_message private, or
change its name to receive_message2 to get rid of the circular call-crash.

Anyways, unfortunately not everything is Windows fault.

Zam.
