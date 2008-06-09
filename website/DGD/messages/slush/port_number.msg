From: "Felix A. Croes"
Subject: Re: [DGD] Port number in driver object..
Date: Thu Jan  6 17:40:01 2005

> As far as I know the port number on which a client connects to DGD should be
> passed to the kernel driver object, but somehow it always ends up being "0".
> Is this a bug or am I missing something? I thought it might be a Win32
> issue, but compiling DGD on the BSD platform had the same effect.

It's a 0 because it's the first port in the array of ports in the config
file.  Use status()[ST_TELNETPORTS] or status()[ST_BINARYPORTS] to map it
to an actual TCP/IP port number.

Regards,
Dworkin
