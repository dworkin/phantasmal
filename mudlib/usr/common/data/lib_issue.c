/* Released into the public domain Jan 2002 by Noah Gibbs.
   Everything in this file is in the public domain, and is
   released without warranty, guarantee, or even the implication
   of marketability or fitness for a particular purpose.
   If you start a war in Asia with this, don't come crying to
   me. */

#include <config.h>
#include <type.h>
#include <status.h>

inherit ISSUE_LWO;

int*  children;
int   marked;

static void create(varargs int clone) {
  ::create(clone);
}

/* This must be passed the owner string, the path to the object (not
   object itself, you can't for a lib), the index returned by status(),
   an array called inherited of issue LWOs and names (for objects
   with no current issue), and the previous issue, if any, of this
   object. */
void set_vals(string owner, string path, int index, mixed* inherited,
	      string* depends_on, int prev) {
  mixed* inherit_from;
  int ctr, tmp_index;
  object issue;

  inherit_from = inherited[..];
  children = ({ });
  marked = 0;

  ::set_vals(index, owner, path, inherit_from,
	     depends_on, prev);
}

void add_child(int child) {
  children += ({ child });
}

void remove_child(int child) {
  int ctr;

  for(ctr = 0; ctr < sizeof(children); ctr++) {
    if(child == children[ctr]) {
      children = children[..ctr-1] + children[ctr+1..];
      return;
    }
  }

  error("Couldn't remove child #" + child + " from " + ob_name
	+ " issue " + index);
}

int* get_children(void) {
  return children;
}

void clear_children(void) {
  children = ({ });
}
