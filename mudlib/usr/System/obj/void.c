#include <phantasmal/lpc_names.h>

/* void.c:
 *
 * The Void is the first room created, and holds space for creators to
 * play around.
 */

inherit ROOM;

#define NEW_PHRASE(a) PHRASED->new_simple_english_phrase(a)

static void create(int clone) {
  ::create(clone);
  if(clone) {
    bdesc = NEW_PHRASE("The Void");
    gdesc = NEW_PHRASE("The Primordial Void");
    ldesc = NEW_PHRASE("The wild wind howls through an empty maelstrom...");
    edesc = NEW_PHRASE("Ooh, distant spots!  No, that's your eyesight going.");

    MAPD->add_room_object(this_object());
  }

}
