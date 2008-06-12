From: dgd at list.imaginary.com (Michael McKiel)
Date: Thu Feb  5 09:19:01 2004
Subject: [DGD] Offline Editing & TF

This is more an FYI since I've seen the editing question pop up so much in
the Archives for the last 3 years (haven't gotten through the ones I
downloaded via ftp yet -- the web ones are so much easier to peruse :)

But this is something that we've been using so far, since the other Admin is
hosting it locally on his machine, and I as frequently edit offline as in a
shell acct on the machine, or in ed on the mud itself.

Anyways, a functionality that TinyFugue has is the /quote command.
One thing you should be aware of is that any line beginning with "/" in
default TF will be considered a TinyFugue command. But there is a way to
override this. I have successfully used "/quote" to upload files > 20KB thus
far. While connected to the mud, I start an editor up "ed" then "a"
/quote /send '<directorylocaltoTinyFugue>/fileName

In my instance the dir TF is usally in would be ~/tf-dir/, so if I am
uploading from say ~/tf-dir/mudcode
I would do: /quote /send 'mudcode/somefile.c
(btw the "/send" is what negates TF from trying to use a line starting with
"/" as a TF command.)

what I also do is put a "." as the first char on the last line of the file
and a "w" then a "q"
so when the transmission is ended it writes out the file and quits the ed.
The other thing to note is /set ptime 0 ... else every "line" of the file
being transmitted will take 1 second ;)

Anyways, I know not if this is helpful to anyone but TF is also available as
a WinBlows precompile www.druware.com/downloads/  -- for anyone that doesn't
want to compile in Cygwin.

Sorry for the non-DGD-related spam...just seemed to relate to a lot of the
embedding editor and ftp/etc queries that people have asked over the years.
