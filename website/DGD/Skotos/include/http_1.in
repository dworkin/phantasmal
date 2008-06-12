From: "Christopher Allen"
Subject: RE: [DGD] DGD http daemon?
Date: Wed Feb  9 09:40:01 2005

On Tuesday, February 08, 2005 12:27 PM Stephen Schmidt <> wrote:
> I gather this daemon runs alongside of the mud, but the players still
> connect and play via the telnet protocol. 
> Could this daemon be extended so that the players played primarily
> through the http protocol, probably using the telnet side only for
> communications (which are hard to implement in http)?  
> 
> There are all kinds of design-related problems with using a primarily
> http interface for playing the game; those don't interest me so much.
> The question is really whether this daemon could be the basis for
> code support for such a design. If it is, then having a version for a
> general DGD kernel (or even a specific kernel that could be released
> as a package with the daemon) would be extremely useful.     

There are some advantages for simulating telnet through http -- you can then
work with proxies and such. We've had a lot of problems with players
attempting to connect from work, for instance. For now, many cheap firewalls
don't block port 443 (https) so that relieves some of the problem. I know of
at least one company that did that, faked http traffic enough that stateful
firewall proxies would pass it on, but to the user was just like telnet.

A different issue is interface in a web world. You should take a look at the
Zealous and Alice clients that we use at Skotos -- try out Castle Marrach
(free trial) using IE (for Alice) or Mozilla/Firebird (for Zealous) to see
how we use a combination of a telnet client embedded inside a web page, that
can call the web page and change it, and the web page can call the telnet
client as well. For one of the best examples of this, try Grendel's Revenge
(IE only for right now.)

I've said for a while elsewhere that we would be glad to share both clients.

-- Christopher Allen
