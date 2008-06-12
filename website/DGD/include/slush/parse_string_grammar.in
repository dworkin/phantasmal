From: Josh Dady
Subject: [DGD] LPC parse_string grammar
Date: Sun, 26 Sep 2004 18:59:04 -0400

-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1

One of my little side projects is to start generating reference 
documentation using doxygen.  Since there's not native support for LPC 
there, it seemed like the simplest way to get there was to use 
parse_string() and spit out C++ declarations equivalent to the LPC 
originals and use doxygen on that.

To that end, while trying to parse our player object, I found the 
following change to the grammar was needed (and Erwin seemed to agree 
that it was something that happened since the grammar was originally 
written) -- one of the varargs cases.  It is legal to use varargs in 
the list of arguments to a function, and the original grammar doesn't 
permit this.  I added the following production to make it happy:

	formal_declaration: 'varargs' type_specifier data_dcltr

- --
Joshua P. Dady
http://www.indecisive.com/
