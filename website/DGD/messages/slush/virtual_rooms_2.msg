From: DGD Mailing List (Felix A. Croes)
Date: Wed Aug 25 08:42:01 2004
Subject: [DGD] Virtual Rooms

Thomas Rice wrote:

> I was wondering if anyone knew how I could implement 'virtual rooms'
> in DGD. Specifically the type that would let me generate a large
> two-dimensional terrain where the room description would be set by
> some equation.
>[...]
> With this method you could easily create a large wilderness area by
> creating a file like "thomas/wilderness.c" and referring to rooms like
> "thomas/wilderness/1,1,1" and having the GetVirtualObject parse the
> string to work out the coordinates and then set up the room by cloning
> it and returning it.

Simple enough.  Either clone rooms in the area and refer to them using
your own custom naming scheme (e.g. "thomas/wilderness"->room_at(1,1,1))
or compile the room from a string instead of a file, and then
configure it as a clone:

    room = compile_object("thomas/wilderness/1,1,1",
			  read_file("thomas/wilderness.c"));
    room->set_long(etc);

The latter will require a recent version of DGD.

Regards,
Dworkin
