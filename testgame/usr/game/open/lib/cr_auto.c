#include <phantasmal/lpc_names.h>
#include <gameconfig.h>

inherit room ROOM;

static void create(varargs int clone) {
  room::create(clone);

  bdesc = PHR("a SoN room");
  ldesc = PHR("You see a room here.  It appears to be part of the game\n"
	      + "Seas of Night.");
  edesc = nil;

  MAPD->add_room_object(this_object());
}

void destructed(varargs int clone) {
  room::destructed(clone);
}

void upgraded(varargs int clone) {
  room::upgraded(clone);
}
