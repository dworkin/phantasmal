Date: Tue, 31 Oct 2000 13:00:06 +0100 (CET)
From: "Felix A. Croes"
Subject: Re: [DGD]inherit_program() called twise

> I wondering if it is normal for inherit_program() in driver object to be called twise each time an object (that inherits something) is compiled.

Twice, or more often: that is normal.  DGD's compiler is not reentrant,
so if an inherited object is discovered not to exist, the current
compilation is aborted and the non-existing object is compiled first.
After that, the original object is compiled again, and inherit_program()
is called again.

Regards,
Dworkin

List config page:  http://list.imaginary.com/mailman/listinfo/dgd
