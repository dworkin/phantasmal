#include <phantasmal/lpc_names.h>
#include <gameconfig.h>

inherit room ROOM;
inherit unq  DTD_UNQABLE;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);

  bdesc = PHR("a SoN room");
  ldesc = PHR("You see a room here.  It appears to be part of the game\n"
	      + "Seas of Night.");
  edesc = nil;

  MAPD->add_room_object(this_object());
}

static void destructed(varargs int clone) {
  room::destructed(clone);
  unq::destructed(clone);
}

static void upgraded(varargs int clone) {
  room::upgraded(clone);
  unq::upgraded(clone);
}
