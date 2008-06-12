From: dgd at list.imaginary.com (Jay Shaffstall)
Date: Thu Mar 21 12:06:01 2002
Subject: [DGD] Printing out nil?

>When you find out what makes the difference between crashing and not
>crashing, please let me know.  DGD should never crash.

Okay, here it is.  It was a silly error on my part, in my logging daemon:

         #include <config.h>
         #include <std.h>

         private int next;

         void log (string message)
         {
                 /* For now, use a generic log file...later use specific ones
                    for specific types of messages */

                 if (! next)
                 {
                         log ("Start of run-------------------\n");
                         next = 1;
                 }

                 write_file (LOG_DIR + "/log.txt", "\n" + message + "\n");
         }

The first time in, a recursive call to itself it made.  However, the flag 
to stop the recursion isn't set first, so I was getting an infinite 
recursion, which ended in a DGD crash.  Since I was logging compile and 
runtime errors to a file, any compile or runtime error would seemingly 
cause the crash.

Not sure what you can do in DGD about this type of thing, since it's 
impossible to tell the difference between infinite recursion and a 
recursion that simply isn't finished yet (at least until you run out of 
stack space).

Hope this helps,
Jay
