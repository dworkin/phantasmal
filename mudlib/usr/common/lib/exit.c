#include <exit.h>
#include <config.h>
#include <type.h>
#include <log.h>
#include <phrase.h>
#include <map.h>
#include <obj_flags.h>

#include <kernel/kernel.h>

inherit OBJECT;

/* exit.c

   Inherited by any exit type.  An object which is an exit might contain
   or inherit from one, though that design isn't set in stone yet...
*/

int    direction;           /* 8 compass directions */
int    type;           /* one-way, two-way, etc. */
int    link_to;             /* exit # that a two-way links to */
object from_location;
object destination;

/* Object flags uses flag values in obj_flags.h */
private int objflags;

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

void set_exit_type(int new_type) {
  if(previous_program() == EXITD) {
    switch (new_type) {
      case ET_ONEWAY:
      case ET_TWOWAY:
              type = new_type;
              break;
      default: /* unknown type */
              type = ET_TWOWAY;
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

int get_exit_type() {
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

/*
 * flag overrides
 */

int is_container() {
  return objflags & OF_CONTAINER;
}

int is_open() {
  return objflags & OF_OPEN;
}

int is_openable() {
  return objflags & OF_OPENABLE;
}

int is_locked() {
  return objflags & OF_LOCKED;
}

int is_lockable() {
  return objflags & OF_LOCKABLE;
}

private void set_flags(int flags, int value) {
  if(value) {
    objflags |= flags;
  } else {
    objflags &= ~flags;
  }
}

/* These may seem a little weird.  The problem is, we need to access
 * a different objects private function.  The recursion takes care
  * of this for us */
void set_container(int value) {
  object link_exit;

  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as a container!");

  if (get_link()!=-1 && previous_program() != EXIT) {
    link_exit = EXITD->get_exit_by_num(get_link());
    link_exit->set_container(value);
  }

  set_flags(OF_CONTAINER, value);
}

void set_open(int value) {
  object link_exit;
  string err;

  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as open!");

  if (get_link()!=-1 && previous_program() != EXIT) {
    link_exit = EXITD->get_exit_by_num(get_link());
    /* why is this necessary on boot-up? */
    err = catch ( link_exit->set_open(value) );
  }

  set_flags(OF_OPEN, value);
}

void set_openable(int value) {
  object link_exit;

  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as openable!");

  if (get_link()!=-1 && previous_program() != EXIT) {
    link_exit = EXITD->get_exit_by_num(get_link());
    link_exit->set_openable(value);
  }

  set_flags(OF_OPENABLE, value);
}

void set_locked(int value) {
  object link_exit;

  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as a container!");

  if (get_link()!=-1 && previous_program() != EXIT) {
    link_exit = EXITD->get_exit_by_num(get_link());
    link_exit->set_locked(value);
  }

  set_flags(OF_LOCKED, value);
}

void set_lockable(int value) {
  object link_exit;

  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as open!");

  if (get_link()!=-1 && previous_program() != EXIT) {
    link_exit = EXITD->get_exit_by_num(get_link());
    link_exit->set_lockable(value);
  }

  set_flags(OF_LOCKABLE, value);
}

/* Return nil if a user can pass through a door, the reason if they cannot */
string can_pass(object pass_object) {
  if (!is_open())
    return "The door is closed.";
}

/* Called when a user passes through the exit */
void pass(object pass_object) {
}
