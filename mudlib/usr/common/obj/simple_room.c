#include <phantasmal/phrase.h>
#include <phantasmal/map.h>
#include <phantasmal/log.h>

#include <config.h>

inherit room ROOM;
inherit unq DTD_UNQABLE;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);
  if(clone) {
    bdesc = PHR("a room");
    gdesc = PHR("a room");
    ldesc = PHR("You see a room here.");
    edesc = nil;

    MAPD->add_room_object(this_object());
  }
}

void destructed(int clone) {
  room::destructed(clone);
  unq::destructed(clone);
  if(clone) {
    MAPD->remove_room_object(this_object());
  }
}

void upgraded(varargs int clone) {
  room::upgraded(clone);
  unq::upgraded(clone);
}

string to_unq_text(void) {
  return "~room{\n" + to_unq_flags() + "}\n";
}

void from_dtd_unq(mixed* unq) {
  int ctr, is_room;

  if(unq[0] != "room")
    error("Doesn't look like room data!");

  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    from_dtd_tag(unq[1][ctr][0], unq[1][ctr][1]);
  }
}
