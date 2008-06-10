From dgd@dworkin.nl  Sat Jul 16 14:46:01 2005
From: dgd@dworkin.nl (Felix A. Croes)
Date: Sat Jul 16 13:46:01 2005
Subject: [DGD] DGD Segmentation Fault

"Neil McBride" wrote:

> Whilst playing around with the kernel lib and trying to build my own
> object manager, I inadvertantly put a call_other within its forbid_call
> function.  Upon startup, after having my initd call set_object_manager in
> the driver, I then have it call setup() in the object manager - which then
> triggers the crash ;)
>
> Obviously my code is doomed to fail whilst going in circles, but I would
> have thought the driver might have caught the error and given some sort of
> message.  Is the seg fault intended or have I stumbled across a bug
> through my *cough* fault tolerance testing *cough* ??

The key to this mystery is not how it occurred, but where.

While the kernel library tries to protect you, there are a few places
where code can still run without a limit on stack or ticks usage, and
the object manager is one of them.  Your code crashed the server
because it ran out of stack space.

DGD takes the view that any hard limit on stack or ticks usage is
arbitrary, and instead provides the programmer with the necessary
tools to impose his own limits.

Regards,
Dworkin
