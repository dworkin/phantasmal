#include <config.h>

inherit OBJECT;

/* detail.c - inherited by any detail object.

The detail objects aren't tracked by a DETAILD the way rooms are
tracked by MAPD or exits are tracked by EXITD.  Instead they are
listed in each object that contains details, usually objects tracked
by MAPD.

*/

static void create(varargs int clone) {
  ::create(clone);
  if(clone) {

  }
}

void upgraded(varargs int clone) {
  ::upgraded(clone);
}

void set_number(int new_num) {
  if(previous_program() == MAPD) {
    tr_num = new_num;
  } else error("Only MAPD can set a portable's tracking number.");
}
