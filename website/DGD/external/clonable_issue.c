#include <config.h>
#include <type.h>
#include <status.h>

inherit "/lib/lwo/issue";

private int num_clones;

static void create(varargs int clone) {
  ::create(clone);
}

void set_vals(string owner, object obj, int index, mixed* inherited,
	      object prev, int mod_count) {
  mixed* inherit_from;
  mixed* depends_on;
  int ctr;
  object issue;

  num_clones = 0;

  depends_on = ({ });
  inherit_from = inherited[..];

  ::set_vals(index, owner, object_name(obj), inherit_from,
	     depends_on, prev, mod_count);
}

void add_clone(object obj) {
  num_clones++;
}

void destroy_clone(object obj) {
  num_clones--;
}

int get_num_clones(void) {
  return num_clones;
}

void clones_from(object issue) {
  /* This'll change if we ever keep track of individual clones... */
  num_clones += issue->get_num_clones();
}
