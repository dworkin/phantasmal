#include <config.h>

inherit PORTABLE;

#define NEW_PHRASE(a) PHRASED->new_simple_english_phrase(a)

static void create(varargs int clone) {
  ::create(clone);

  if(clone) {
    bdesc = NEW_PHRASE("a player");
    gdesc = NEW_PHRASE("an unknown player");
    ldesc = NEW_PHRASE("A crudely-formed anonymous player wanders the MUD.");
    edesc = NEW_PHRASE("This player appears not to have ever fully logged in,"
		       + " yet here he is!");
  }
}

void set_player_name(string new_name) {
  bdesc = NEW_PHRASE(new_name);
  gdesc = NEW_PHRASE(new_name);
  ldesc = NEW_PHRASE(new_name + " wanders the MUD.");
  edesc = NEW_PHRASE("Ooh!  You found something special about " + new_name
		     + ".");

  add_noun(PHRASED->new_simple_english_phrase(new_name));
}


void destructed(varargs int clone) {
  ::destructed(clone);
}

void upgraded(varargs int clone) {

}
