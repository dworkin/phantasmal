From: DGD Mailing List (Par Winzell)
Date: Sun Apr  4 09:16:05 2004
Subject: [DGD] Re: Clones and very large arrays

Robert Forshaw wrote:

> Thanks for that, it was very insightful. I have a few questions.
> 
> First of all, the following line:
> 
> get_row(obj)[obj] = TRUE;
> 
> seems to suggest that mappings are passed by reference! Is this the 
> case? I wasn't aware of it.

Yes, they are, at least to some extent. I believe the contract with the 
driver is that references last as long as you stay either inside one 
thread or inside one object. In other words, if you store a reference in 
one object, export it to another, and the thread ends, DGD reserves the 
right to create a private copy of the mapping/array in the other object. 
  So you can't keep long-term references to data inside another object.


> Secondly, I understood everything in the code except the iterator. I 
> know what an iterator is, but that's about it. I wouldn't mind knowing 
> what was in /data/bigmap_iterator, but you haven't revealed that. I also 
> have no idea what the second argument to new_object is supposed to be, 
> as the driver version doesn't have one.

We redefined new_object() in our system-level auto object for 
convenience; it reads something like (again from memory) --

static atomic
object new_object(mixed ob, mixed args...) {
    ob = ::new_object(ob);
    /*
    ** actually this is a simplification; we relay the call through a
    ** _F_configure() function to allow configure() to be static
    */
    ::call_other(ob, "configure", args...);
    return ob;
}

> Thirdly, why 1024? As opposed to any other number.

No particular reason. It means you assume nobody is ever going to set 
DGD's max-array-size parameter lower than 1024, and it means that you 
can store a total of max-array-size * 1024 objects. You could make it 
e.g. 4096 if you preferred. If you mean 1024 rather than e.g. 1000, 1024 
is just a rounder number (2^10).

As for the iterator, there's a lot of ways you could implement it. There 
are two basic kinds of iterators in the world; one that promises it will 
capture the state of the data-structure when iterator() is called and so 
even if the data structure itself changes later, the iterator will still 
enumerate precisely what was there when the snapshot was made...

... and one kind of iterator that makes no such promise. Obviously the 
former kind has to either use more memory, or special tricks. If you 
don't mind the iterator using a fair bit of memory (temporarily) you can 
  simply let the iterator LWO make a private copy of the bigmap, keep 
two index counters (one to index the mapping of mappings, one to index 
the current inner mapping), and step through the whole thing step by 
step as next() is called.

I stole Sun's most recent interface pretty much directly;

   int has_next();
   mixed next();
   int size();


The nice bit about generalized iterators is that you are free to vary 
your internal implementation both of the datastructure and of the 
iterator as you please. All the caller cares about is being returned an 
object that keeps track of the iteration state. For example, the Skotos 
all-clones-of-a-certain-clonable data structure uses a SINGLE mapping 
only. If the number of clones of a clonable grows beyound 1024 or so 
(maybe we use 2048 or 4096), we just empty the data structure (!). Why? 
Because DGD offers a second method to enumerate all the clones of a 
given clonable, namly find_object().

Thus, for clonables with lots of clones, we return an iterator that 
reads basically:


string clonable;
object next;
int ix;

static atomic
void configure(string str) {
   clonable = str;
   ix = 1; forward();
}

int has_next() {
   return !!next;
}

object next() {
   if (next) {
     object ob;

     ob = next;
     forward();

     return ob;
   }
   error("read past end");
}

private
void forward() {
   while (ix <= status()[ST_OTABSZ]) {
     next = find_object(clonable + "#" + ix);
     ix ++;

     if (next) {
       return next;
     }
   }
   next = nil;
}
