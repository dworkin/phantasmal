/* This is the lightweight object (LWO) which the objectd uses to keep
   track of issues of objects -- that is, individual compiled versions
   of them.  There may be more than one issue per object in some
   cases.
*/

/* (originally from objectd.c:)
   The ISSUE_LWO is a structure which contains the info we care about
   for each issue.  That means it stores things like object name,
   issue compile time, and so on.  LWOs may link to each other since
   the group of them exist entirely within the single HEAVY_ARRAY
   (see below).  The ISSUE_LWO is never used directly, but is instead
   used as one of LIB_LWO or CLONABLE_LWO, both of which inherit
   from ISSUE_LWO. */

#include <config.h>

/* The object name - may be shared with other issues of the same object */
string ob_name;
/* The object index -- unique to this issue */
int index;
/* The time this issue was compiled */
private int comp_time;
/* Array of objects inherited from */
private mixed* inherit_from;
/* Array of paths this issue depends on directly other than parents */
private string* depends_on;
/* Previous version, if any, of this issue */
private int prev_index;
/* Whether the object has been destructed */
private int destructed;

static int create(varargs int clone) {
  if(clone) {
    ob_name = "";
    destructed = 0;
  }
}

static void set_vals(int new_index, string new_owner, string new_name,
		     mixed* new_inherit_from,
		     string* new_depends_on, int new_prev_index) {
  if(ob_name != "")
    error("Reinitializing ISSUE object!");

  if(new_name == "" || new_name == nil)
    error("Initializing ISSUE object with invalid name!");

  index = new_index;
  ob_name = new_name;

  comp_time = time();
  inherit_from = new_inherit_from;
  depends_on = new_depends_on;

  prev_index = new_prev_index;
}

string get_name() {
  return ob_name;
}

int get_index() {
  return index;
}

int get_comp_time() {
  return comp_time;
}

mixed* get_dependencies(void) {
  return depends_on;
}

void clear_dependencies(void) {
  depends_on = ({ });
}

void add_dependency(string dep) {
  depends_on += ({ dep });
}

mixed* get_parents(void) {
  return inherit_from;
}

void set_parents(mixed* new_parents) {
  inherit_from = new_parents;
}

void remove_parent(int parent) {
  int ctr;

  for(ctr = 0; ctr < sizeof(inherit_from); ctr++) {
    if(inherit_from[ctr] == parent) {
      inherit_from = inherit_from[..ctr-1] + inherit_from[ctr+1..];
      return;
    }
  }
  error("Cannot remove parent " + parent + " from issue " + index);
}

int get_prev(void) {
  return prev_index;
}

object clear_prev(void) {
  if(previous_program() == OBJECTD) {
    prev_index = -1;
  } else error("Can't set prev from unprivileged program!");
}

void set_prev(int prev) {
  if(previous_program() == OBJECTD) {
    prev_index = prev;
  } else error("Can't set prev from unprivileged program!");
}

void destruct(void) {
  if(destructed)
    error("Object multiply notified of destruction!");

  destructed = 1;
}

int destroyed(void) {
  return destructed;
}
