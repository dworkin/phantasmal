/* This file is the parent object for custom room script objects.
   Note that this library is *not* a Phantasmal room.  Neither are the
   custom room script objects.  That's because their definition
   doesn't have to have anything to do with Phantasmal, though the
   shim object (/usr/game/obj/custom_room) *does* have to be a valid
   Phantasmal room.  With the right sort of shim object, you could
   adapt rooms from other kinds of LPMUD, or even Diku and company.
   However, that'd be a poor way to treat a persistent MUD server. */

#include <kernel/kernel.h>
#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

nomask void dummy_func(void) {

}

/* This opens the object.  Return TRUE to allow, or FALSE to prevent */
int open_script() {
  return TRUE;
}

/* This closes the object.  Return TRUE to allow, or FALSE to prevent */
int close_script() {
  return TRUE;
}

/* This is triggered when the object is picked up.  Return TRUE to allow,
   or FALSE to prevent */
int get_script(object person_getting_this_object) {
  return TRUE;
}

/* This is triggered when the object is dropped.  Return TRUE to allow,
   or FALSE to prevent */
int drop_script(object person_dropping_this_object) {
  return TRUE;
}
