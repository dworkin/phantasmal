From: dgd at list.imaginary.com (Par Winzell)
Date: Sat Apr  3 15:36:07 2004
Subject: [DGD] Re: Clones and very large arrays

Robert,

> So is there anything to stop me from making it an unsigned long, giving 
> me a generous 2.1 million elements or so? I know 2.1 million isn't 
> feasable for obvious reasons, but this is just a cap, right?

The basic reason (as I understand it) there is a limit is because DGD
implements powerful operators in the core language, such as array
addition and string addition, whose execution times are more or less
linearly proportional to the size of the operands (arrays).

Whenever linear operations are used for fundamental operations (clones
of a clonable is a perfect example), scalability goes out the window. 
Making a game truly scale is surprisingly difficult, and pretty much the 
first thing you do is get rid of every single place where you use an 
array to store anything substantial.

If there are N clones of an object and cloning a new object takes time 
proportional to N, then it takes N^2 time to clone all those objects. 
That's the crux of the problem. It means that if your prototype runs 
just fine on one machine with 300 players, you need your production 
release to handle a thousand players, and you're already hovering near a 
quadratic bottleneck, you will need to scale your hardware by a factor 
10 to achieve a tripling of the playerbase. If you want to run the game 
with 3,000 players, you'll need a cluster of one hundred machines.

In reality, this means that a server that uses linear-time 
implementations of fundamental operations will never outgrow the 
prototype stage.

So what do you do?

> I'm also wondering if it would be ok to have an array of arrays storing 
> the clones:
> ({ ({ a_lot_of_clones }), ({ some_more_clones })... })
> 
> Are there any reasons not to do it that way?

That's a possibility, and avoids the full O(N^2) problem, but it's kind 
of clunky. I'd suggest using mappings of mappings for virtually every 
place you store a significant list of anything. We have long since 
written special-purpose LWO's for the Skotos mudlib (called bigmaps) 
that do the grunt work of maintaining sets.

Thus clone-maintenance code looks something like:

   object clones;

   static void create() {
     clones = new_object("/data/ob_bigmap");
   }

   atomic void add_clone(object clone) {
     clones->add(clone);
   }
   atomic void del_clone(object clone) {
     clones->del(clone);
   }
   object iterator() {
     return clones->iterator();
   }


while ob_bigmap.c looks something like (this is from memory, it will 
probably not compile) --

   mapping mapofmaps;

   static void create() {
     mapofmaps = ([ ]);
   }
   private atomic mapping get_row(object ob) {
     mapping row;
     int ix;

     /* I kind of wish status(ob) returned the clone number */
     if (sscanf(object_name(ob), "%*s#%d", ix) != 1) {
       ix = status(ob)[O_INDEX];
     }
     row = mapofmaps[ix / 1024];
     if (row == nil) {
       row = mapofmaps[ix / 1024] = ([ ]);
     }
     return row;
   }

   atomic void add(object ob) {
     get_row(ob)[ob] = TRUE;
   }
   atomic void del(object ob) {
     get_row(ob)[ob] = nil;
   }
   object iterator() {
     return new_object("/data/bigmap_iterator", mapofmaps);
   }


I will leave the iterator as an excercise for the reader. :)

Zell
