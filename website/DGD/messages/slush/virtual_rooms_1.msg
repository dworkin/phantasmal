From: DGD Mailing List (Thomas Rice)
Date: Wed Aug 25 07:16:01 2004
Subject: [DGD] Virtual Rooms

Hi,

I was wondering if anyone knew how I could implement 'virtual rooms'
in DGD. Specifically the type that would let me generate a large
two-dimensional terrain where the room description would be set by
some equation.

Here is how a previous version of a LpMUD derivative I once used
accomplished this.

When you tried to go to a particular location or load a particular
object, such as "thomas/room/hallway.c" it would do 3 things, in this
order:

1. It would check if it's already loaded in memory. If so, it would use that.
2. If not, it would load up hallway.c and use that.
3. If hallway.c didn't exist, it would check to see if "thomas/room.c"
existed, and if so call the GetVirtualObject function within
"thomas/room.c" with the full string as an argument. If this function
existed, it would return a room object, and in memory rename it to
"thomas/room/hallway.c" so that future references to this would refer
to the loaded room.
4. If hallway.c didn't exist, or the GetVirtualObject function didn't
return a room, it would fail.

With this method you could easily create a large wilderness area by
creating a file like "thomas/wilderness.c" and referring to rooms like
"thomas/wilderness/1,1,1" and having the GetVirtualObject parse the
string to work out the coordinates and then set up the room by cloning
it and returning it.

Does anyone know if it's possible to do something similar in DGD, and
if so, how? I'm pretty new to DGD, I should point out, but so far it
looks pretty good! :)

Thanks,

Thomas.
