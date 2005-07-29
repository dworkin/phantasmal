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

inherit room ROOM;
inherit unq DTD_UNQABLE;

/* Override how create/destructed/upgraded work! */

nomask static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);

  if(clone) {
    MAPD->add_room_object(this_object());
    if(function_object("room_created", this_object())) {
      this_object()->room_created();
    }
  } else {
    GAME_ROOM_REGISTRY->add_room_type();
  }
}

nomask void destructed(varargs int clone) {
  if(!SYSTEM())
    error("Can't call room::destructed() unprivileged!");

  room::destructed(clone);
  unq::destructed(clone);
  if(clone) {
    MAPD->remove_room_object(this_object());

    if(function_object("room_destructed", this_object())) {
      this_object()->room_destructed();
    }
  }
}

nomask void upgraded(varargs int clone) {
  if(!SYSTEM())
    error("Can't call room::upgraded() unprivileged!");

  room::upgraded(clone);
  unq::upgraded(clone);

  if(function_object("room_upgraded", this_object())) {
    this_object()->room_upgraded();
  }
}

string to_unq_text(void) {
  if(SYSTEM() || COMMON() || GAME()) {
    string filename;

    if(!sscanf(object_name(this_object()), GAME_ROOMS_DIR + "%s", filename)) {
      error("Can't parse object name " + object_name(this_object())
	    + " in to_unq_text!");
    }
    return "~object{\n  ~obj_type{custom:/" + filename + "}\n"
      + to_unq_flags()
      + "\n}\n\n";
  }
  error("Can't call to_unq_text unprivileged!");
}

void from_dtd_unq(mixed* unq) {
  int    ctr;
  string dtd_tag_name;
  mixed  dtd_tag_val;

  if(!SYSTEM() || !COMMON() || !GAME())
    error("Can't call custom_room:from_dtd_unq unprivileged!");

  if(unq[0] != "object")
    error("Doesn't look like object data!");

  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    dtd_tag_name = unq[1][ctr][0];
    dtd_tag_val  = unq[1][ctr][1];

    if(dtd_tag_name == "data") {
      /*
      if((typeof(dtd_tag_val) != T_ARRAY)
	 || (sizeof(dtd_tag_val) != 2)
	 || (dtd_tag_val[0] != "filename"))
	error("Error in data section of UNQ for custom room obj!");

      set_filename(dtd_tag_val[1]);
      */
    } else {
      from_dtd_tag(dtd_tag_name, dtd_tag_val);
    }
  }
}
