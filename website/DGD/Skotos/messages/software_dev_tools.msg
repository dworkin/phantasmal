From: dgd@dworkin.nl (Par Winzell)
Date: Tue May  3 16:38:01 2005
Subject: [DGD] Software Development Tools

Neil McBride wrote:
> Hi all,
> 
> It's been a while since I've poked my head up in here - hope you're all
> doing well ;)  I've been thinking lately about software development tools,
> preferably linux but maybe the evil M$ side if there's something good
> enough there.
> 
> I find that the way I tend to work on my own little mud project is about
> once every few months or more, and lately whenever I go to do some more
> work on it, I find myself working out where I was up to, what I was
> thinking, etc.  In spite of somewhat excessive comments in code and the
> half-written design docs, I still find myself going back to the start for
> a refresher course on what I've done.

For SkotOS 2.0, I use --

 * Subversion for source control. RIP CVS. IMO this is the only
significant alternative at the moment.
 * Mantis for bug tracking. This is invaluable to me, for the reason you
mention... The only realistic way for me to keep discipline is to have a
place where I can go to add a bug note whenever I work a bit on some
bug. If I keep it in my head, it's lost by the time I come back to the
project. Do not use Bugzilla!
 * Doxygen for documentation. Doxygen out of the box does an excellent
job at parsing LPC, and even without any actual documentation, Doxygen
can build you a ton of really gorgeous pages cross-referencing your code
every which way. Adding a little LPC-specific pre-processor would let it
keep track of subclassing structures and such, too.
 * Emacs remains the editor of choice, of course.

Zell
