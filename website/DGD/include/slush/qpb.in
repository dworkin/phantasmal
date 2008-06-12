From: dgd@dworkin.nl (will guaraldi)
Date: Tue Mar 22 16:36:01 2005
Subject: [DGD] Question for programmers and builders

On Mon, 21 Mar 2005, Noah Gibbs wrote:
>
> Phantasmal gets a lot of flak for being gratuitously weird, by which I 
> mean "not like 2.4.5".  And that's true, it really isn't.  Would people 
> have an easier time with passing Phrases around if they looked like the 
> markup strings above instead of looking like opaque LWO structures that 
> you have to call functions on?

I don't use Phantasmal or DGD, though I've been following development of 
both for a few years now.  Even so, I don't really understand what you're 
talking about here.

However, one thing I do want to mention is that I frequently use ispell to 
spellcheck descriptions and the like and I frequently run it over code 
files.  So whatever solution you end up with, keep in mind that people may 
use other non-mud tools to do their work so the most text-centric approach 
for building objects is definitely the better one.

In regards to storing that internally, you could store a version of the 
expanded string (i.e. the string with all the markup code expanded into 
ANSI/HTML/Xterm/...) for each language.  But I would think that gets 
pretty memory intensive if every phrase consisted of a markup version for 
each language supported.  (2 languages and 3 markup formats would be 6 
strings stored for each phrase).  Also, you'd spend a lot of time 
expanding all those strings to all the variations.

I think the issue here is memory and how long it takes to expand strings. 
You clearly want to limit the number of times you're expanding the same 
string, but on the flip side, you don't want to go expand everything and 
suffer the memory penalties.  Maybe it's better to cache the expanded 
strings after the first time they've been requested?

The other problem I've had that would be interesting to solve is trying to 
use marked up strings for comparison.  This goes along with the problem 
where people may adjust the phrase after the fact.  Is there a way to do 
string comparisons on "~green(green) apple" without having to worry about 
the ~green(...) part?  It would be nice if "~green(green) apple" == "green 
apple".

I'm doing a lot of talking about things I know next to nothing about, so 
I'm going to cut it short here.

I think it's pretty wild that you could have a mud written in multiple 
languages.  I'm not sure how that would really work.  Is there a need for 
such a thing?  How do players of such muds interact with each other?

/will
