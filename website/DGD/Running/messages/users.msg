From: dgd at list.imaginary.com (Erwin Harte)
Date: Mon Nov 12 11:39:01 2001
Subject: [DGD] Users

On Mon, Nov 12, 2001 at 06:18:13PM +0100, Troels Therkelsen wrote:
[...]
> 
> Someone more into the black magic of the internals of the DGD driver will
> have to answer what the max value you can safely set users to is.

DGD will let you go up to whatever EINDEX_MAX is:

  ---- Quote from dgd/src/config.c ----
                                { "users",              INT_CONST, FALSE,
                                                        1, EINDEX_MAX },
  ----

In config.h you'll see that EINDEX_MAX is UCHAR_MAX, which I would
assume to be 255?

Of course if you change the typedef for eindex to something bigger and
change the EINDEX_MAX and EINDEX defines then you can go further, as
long as your operating system or user-limits don't stop you sooner.

Regards,

Erwin.
-- 
Erwin Harte
