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
  return "~object{\n" + to_unq_flags() + "\n}\n\n";
}

void from_dtd_unq(mixed* unq) {
  int    ctr;
  string dtd_tag_name;
  mixed  dtd_tag_val;

  if(unq[0] != "object")
    error("Doesn't look like object data!");

  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    dtd_tag_name = unq[1][ctr][0];
    dtd_tag_val  = unq[1][ctr][1];

    if(dtd_tag_name == "data") {
      /* This is meaningful for other object subtypes, perhaps, but
	 not for us.  Skip it. */
    } else {
      from_dtd_tag(dtd_tag_name, dtd_tag_val);
    }
  }
}
