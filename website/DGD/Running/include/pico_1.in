From: dgd at list.imaginary.com (Kirk Turner)
Date: Sun May  4 23:45:01 2003
Subject: [DGD] Implement Pico Editor

Lord Lerkista wrote:
> Hi, i'm trying to implement Pico as the editor for my Mud, but it have a 
> problem
> i don't know how to send the editor to the player connection, if i call 
> pico from
> the mud the editor opens in the shell where i run the Mud, anyone have 
> tried
> that?? I need help
> 
> _________________________________________________________________
> List config page:  http://list.imaginary.com/mailman/listinfo/dgd
> 
> 

At what level are you trying to implement pico as your editor?

I once thought about doing this sort of thing (using vi, not pico) and 
realised that its not possible because mud connections are in sending a 
a text stream, and unless they have a pico editor locally installed and 
pico has some way of interacting with a text stream then its not 
possible. Ed on the other hand is simply a read and put text interface.

Saying this I suppose it would be possible to incorporate pico into the 
mud because you can run other ncursers programs through telnet 
connections (eg midnight commander), but I would suppose there countless 
  problems with this due timing issues and such (someone with ncursers 
experience might be more help here)

The basic conclusion was that without hacking into vi or pico and 
integrating a mud client into it that reads the file and creates a local 
copy to edit it wasn't possible. The best solution in that sense is to 
use tf (or the like) to silently read the file into a local copy, launch 
pico to edit it in another window, and then have another macro to send 
it back... or my preferred option is to use emacs and implement a ftp 
connection to the mud.

Kirk
