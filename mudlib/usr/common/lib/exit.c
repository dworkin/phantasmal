#include <config.h>

inherit OBJECT;

/* exit.c

   Inherited by any exit type.  An object which is an exit might contain
   or inherit from one, though that design isn't set in stone yet...
*/

int    direction;           /* 8 compass directions */
int    type;                /* one-way, two-way, etc. */
int    link_to;             /* exit # that a two-way links to */
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
  } else error("Only EXITD can set exit directions!");
}

void set_from_location(object new_loc) {
  if(previous_program() == EXITD) {
    from_location = new_loc;
  } else error("Only EXITD can set exit from locations!");
}

void set_destination(object new_dest) {
  if(previous_program() == EXITD) {
    destination = new_dest;
  } else error("Only EXITD can set exit destinations!");
}

void set_type(int new_type) {
  if(previous_program() == EXITD) {
    switch (new_type) {
      case 1: /* one way */
      case 2: /* two way */
              type = new_type;
              break;
      default: /* unknown type */
              type = 2;
              break;
    }
  } else error("Only EXITD can set exit types!");
}

void set_link(int new_link) {
  if(previous_program() == EXITD) {
    link_to = new_link;
  } else error("Only EXITD can set links!");
}

int get_direction() {
  return direction;
}

int get_type() {
  return type;
}

int get_link() {
  return link_to;
}

object get_from_location() {
  return from_location;
}

object get_destination() {
  return destination;
}

/* Return nil if a user can pass through a door, the reason if they cannot */
string can_pass(object pass_object) {
  if (!is_open())
    return "The door is closed.";
}

/* Called when a user passes through the exit */
void pass(object pass_object) {
}
