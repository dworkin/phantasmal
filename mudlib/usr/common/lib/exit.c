#include <config.h>

inherit OBJECT;

/* exit.c

   Inherited by any exit type.  An object which is an exit might contain
   or inherit from one, though that design isn't set in stone yet...
*/

int    direction;
object from_location;
object destination;

static void create(varargs int clone) {
  ::create(clone);
  if(clone) {

  }
}

void upgraded(varargs int clone) {
  ::upgraded(clone);
}

void set_number(int new_num) {
  if(previous_program() == EXITD) {
    tr_num = new_num;
  } else error("Only EXITD can set exit numbers!");
}

void set_direction(int new_dir) {
  if(previous_program() == EXITD) {
    direction = new_dir;
  } else error("Only EXITD can set exit numbers!");
}

void set_from_location(object new_loc) {
  if(previous_program() == EXITD) {
    from_location = new_loc;
  } else error("Only EXITD can set exit numbers!");
}

void set_destination(object new_dest) {
  if(previous_program() == EXITD) {
    destination = new_dest;
  } else error("Only EXITD can set exit numbers!");
}

int get_direction() {
  return direction;
}

object get_from_location() {
  return from_location;
}

object get_destination() {
  return destination;
}

/* Return nil if a user can pass through a door, the reason if they cannot */
string can_pass(object pass_object) {
  return nil;
}

/* Called when a user passes through the exit */
void pass(object pass_object) {
}
