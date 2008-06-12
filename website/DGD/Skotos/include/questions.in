From: dgd@dworkin.nl (Par Winzell)
Date: Tue Mar 22 16:28:01 2005
Subject: [DGD] Question for programmers and builders

Noah Gibbs wrote:

>   Phantasmal gets a lot of flak for being gratuitously weird, by which I mean
> "not like 2.4.5".  And that's true, it really isn't.  Would people have an
> easier time with passing Phrases around if they looked like the markup strings
> above instead of looking like opaque LWO structures that you have to call
> functions on?

I think Felix already supplied the vital distinction here... Add, for 
example, a mudlib-wide interface for editable objects that specify 
something like

   string export_ascii_state();
and
   void import_ascii_state(string state);

and use these in any situation where a developer edits any object 
through any text-based interface (telnet, web, ...).

The Skotos mudlib does a lot of this. Merry scripts are stored in a 
half-compiled form (from which the original source can quickly be 
reconstructed for editing purposes).

SAM -- which is marked-up text much like what you're implementing -- is 
similarly a "binary" format internally and presented in a text form to 
the various user interfaces.

To demonstrate a final point in favour of the above, let me outline one 
of the Skotos mudlib decisions I wish we'd never made -- for out final 
output post-processing step -- the markup that is not evaluated until 
it's just about to be sent out to the user, and thus is 100% subjective 
to that user -- we foolishly opted to use LPC strings directly, 
marked-up with otherwise unused characters.

Thus if I exclaim something in Dwarfish, the resulting output for 
onlookers would be something like,

   TAG("Zell exclaims in Dwarfish, \'" +
           INLANG("My axe is my best friend!", "dwarfish") + "\"",
       "Social")

which turns not into an LWO or anything, but a rather confusing string:

   \000\003Social\001\000\206\dwarfish|7\001
   "My axe is my best friend!"\002\002

which is then sent to the user object, which does the final parsing and 
manipulation. The tags are looked up in the user's profile and turned 
into e.g. font colour settings -- so they can have all social emotes 
show up in blue, or whatever -- whereas the language data is used to 
garble the string depending on the proficiency in the language of the 
speaker as well as the proficiency of the listener.

The format is extremely quick to parse with parse_string() and it has 
the one advantage of letting developers continue to use string addition. 
  For us, its main advantage was that it could be grafted onto the 
existing mudlib structure without a massive rewrite.

Still, it was a bad idea. Passing these pseudo-strings off as real 
strings is very problematic. Given any string containing interesting 
data, a developer is always going to try to parse it or split it up or 
insert things into it, etc, and this sort of markup is very fragile. 
Passing interesting yet fragile data that looks like strings but isn't 
through developer hands is a recipe for disaster.

So. Always abstract the implementation from the interface.

Zell
