From: dgd at list.imaginary.com (Par Winzell)
Date: Sat Apr  3 15:42:11 2004
Subject: [DGD] Clones and very large arrays

> I'm using a linked list.  This distributes the data across more objects (to
> mitigate swapping concerns) and avoids any issues with respect to max array
> size.  Though obviously getting the actual list of all clones isn't anywhere
> near as fast as it would be by your method.  I don't find a need to get that
> information too often, though.

The problem with this approach is that in my experience, minimizing 
swapping is your primary concern, especially now that call_touch() 
actually enables us to affect objects without swapping them in. If you 
use a linked list where the next/prev pointers are stored in the actual 
objects, then to even enumerate all the clones of a clonable, you have 
to swap them all in. Swapping a quarter of a million objects into memory 
is not realistic, and if you do it lazily (i.e. try not to freeze your 
game) it takes hours and hours.

Zell
