#include <config.h>
#include <type.h>
#include <status.h>

inherit "/lib/lwo/issue";

static void create(varargs int clone) {
  ::create(clone);
}

/* This must be passed the owner string, the path to the object (not
   object itself, you can't for a lib), the index returned by status(),
   an array called inherited of issue LWOs and names (for objects
   with no current issue), and the previous issue, if any, of this
   object. */
void set_vals(string owner, string path, int index, mixed* inherited,
	      object prev, int mod_count) {
  mixed* inherit_from;
  mixed* depends_on;
  int ctr, tmp_index;
  object issue;

  depends_on = ({ });
  inherit_from = inherited[..];

  ::set_vals(index, owner, path, inherit_from,
	     depends_on, prev, mod_count);
}
