#include <config.h>

inherit port ROOM;
inherit unq UNQABLE;

/* A simple portable object */

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  port::create(clone);
  unq::create(clone);
  if(clone) {
    set_brief(PHR("A body"));
    set_glance(PHR("A new body"));
    set_look(PHR("Ahh...  An uninitialized body!"));

    MAPD->add_room_object(this_object());
  }
}

void upgraded(varargs int clone) {
  port::upgraded(clone);
  unq::upgraded(clone);
}

void destructed(varargs int clone) {
  port::destructed(clone);
  unq::destructed(clone);
}

string to_unq_text(void) {
  string ret;
  ret = "~body{\n";
  ret += to_unq_flags();
  ret += "}\n";
  return ret;
}

void from_dtd_unq(mixed* unq) {
  int ctr;

  if(unq[0] != "body")
    error("Doesn't look like a body data!");

  /* load text */
  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    from_dtd_tag(unq[1][ctr][0], unq[1][ctr][1]);
  }
}

/*
 * overloaded room notification functions
 */

string can_get(object mover, object new_env) {
  return "You can't pick up a person!";
}

string can_enter(object enter_object, int dir) {
  return "You can't enter a person!  Don't be silly.";
} 

string can_leave(object leave_object, int dir) {
  return "You can't leave a person!  In fact, you shouldn't even be here.";
}
