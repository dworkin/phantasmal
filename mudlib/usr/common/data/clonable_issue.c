/* Released into the public domain Jan 2002 by Noah Gibbs.
   Everything in this file is in the public domain, and is
   released without warranty, guarantee, or even the implication
   of marketability or fitness for a particular purpose.
   If you start a war in Asia with this, don't come crying to
   me. */

#include <phantasmal/lpc_names.h>
#include <type.h>
#include <status.h>

inherit ISSUE_LWO;

private int num_clones;

static void create(varargs int clone) {
  ::create(clone);
}

void set_vals(string owner, object obj, int index, mixed* inherited,
	      string* depends_on) {
  mixed* inherit_from;
  int ctr;
  object issue;

  num_clones = 0;
  inherit_from = inherited[..];

  ::set_vals(index, owner, object_name(obj), inherit_from,
	     depends_on, -1);
}

void add_clone(object obj) {
  num_clones++;
}

void destroy_clone(object obj) {
  num_clones--;
}

void clear_clones(void) {
  num_clones = 0;
}

int get_num_clones(void) {
  return num_clones;
}

void clones_from(object issue) {
  /* This'll change if we ever keep track of individual clones... */
  num_clones += issue->get_num_clones();
  issue->clear_clones();
}
