From: dgd at list.imaginary.com (Erwin Harte)
Date: Wed Dec 24 11:21:01 2003
Subject: [DGD] 4 bytes should be enough for anyone.

Nice gotcha I ran into this morning, thought it would be worth sharing
it and its solution with others:

I use a heuristic to occasionally force a swapout() when a lot of
memory has been allocated but a significant amount is not in use
(anymore), to avoid cluttering up the memory from the operating
system's point of view.

The condition I used is (simplified):

    if (mem_used < 2 * mem_allocated / 3) {
        /* trigger swapout */
    }

So if I have 120MB allocated and less than 80MB is in use, it'll do
that quite nicely.

However, this morning the memory usage rose to about 1.5GB (1513MB to
be precise) and strangely enough the swapout didn't trigger.  Closer
inspection revealed that:

    1513 * 1024 * 1024 = 1586495488

    1586495488 * 2 = -1121976320

    1586495488 * 2 / 3 = -373992106

Uhm, oops, we ran into the limit of integers and since memory usage is
never going to be negative, it would happily continue to drag those
1.5GB of virtual memory around.

This should work without running into this particular problem:

    if (mem_used / 2 < mem_allocated / 3) {
        /* trigger swapout */
    }

:)

Cheers,

Erwin.
-- 
Erwin Harte
