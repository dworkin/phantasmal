/* This is the lightweight object (LWO) which the OBJECT_MANAGER uses to
   keep track of issues of objects -- that is, individual compiled versions
   of them.  There may be more than one issue per object in some cases.
*/

#include <config.h>

/* The object name - may be shared with other issues of the same object */
string ob_name;
/* The object index -- unique to this issue */
int index;
/* The object's owner */
string owner;
/* The time this issue was compiled */
private int comp_time;
/* Array of objects inherited from */
private mixed* inherit_from;
/* Array of paths this issue depends on directly other than parents */
private string* depends_on;
/* Previous version, if any, of this issue */
private object prev_issue;
/* Whether the object has been destructed */
private int destructed;
/* The mod count, used as a uniquifier for inheritance */
private int mod_count;


static int create(varargs int clone) {
  if(clone) {
    ob_name = "";
    destructed = 0;
  }
}

static void set_vals(int new_index, string new_owner, string new_name,
		     mixed* new_inherit_from,
		     string* new_depends_on, object new_prev_issue,
		     int new_mod_count) {
  if(ob_name != "")
    error("Reinitializing ISSUE object!");

  if(new_name == "" || new_name == nil)
    error("Initializing ISSUE object with invalid name!");

  index = new_index;
  owner = new_owner;
  ob_name = new_name;

  comp_time = time();
  inherit_from = new_inherit_from;
  depends_on = new_depends_on;

  prev_issue = new_prev_issue;
  mod_count = new_mod_count;
  if(mod_count < 0)
    error("Invalid mod_count in /lib/lwo/issue:set_vals!");
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

object get_prev(void) {
  return prev_issue;
}

int get_mod_count(void) {
  return mod_count;
}

object get_issue_by_mod(int mod) {
  object index;

  if(mod > mod_count || mod < 0)
    return nil;
  if(mod == mod_count)
    return this_object();

  index = prev_issue;
  while(index) {
    if(index->get_mod_count() < mod)
      return nil;

    if(index->get_mod_count() == mod)
      return index;

    index = index->get_prev();
  }
  return nil;
}

void destruct(void) {
  if(destructed)
    error("Object multiply notified of destruction!");

  destructed = 1;
}

int destroyed(void) {
  return destructed;
}
