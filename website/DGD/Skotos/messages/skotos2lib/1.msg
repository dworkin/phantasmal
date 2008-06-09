From: DGD Mailing List (Par Winzell)
Date: Tue Aug 31 06:51:01 2004
Subject: [DGD] SkotOS 2.0

> On Monday, August 30, 2004, 8:24:24 PM, Par Winzell wrote:
> PW> It should also be mentioned that the further we get towards 2.0, the
> PW> less of the code is pure LPC. Some of the modules are almost exclusively
> PW> Merry/SAM, especially the web-application ones. Not everybody will need
> PW> to be a hardcore LPC architect.
> 
> I'm curious about this, why implement a new interpreted imperative
> language (Merry) within LPC?  What benefit does it provide over
> straight LPC?

It's not interpreted in LPC. Merry is compiled into actual LPC. There is 
no significant speed loss. The benefits are not so much in the language 
itself but in the fact that it allows little snippets of code to be sent 
around as data, which is a natural way for us to hook logic onto events.

Zell
