#include <phantasmal/phrase.h>
#include <phantasmal/map.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <kernel/kernel.h>

#include <gameconfig.h>
#include <type.h>

inherit room ROOM;
inherit unq DTD_UNQABLE;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

void set_filename(string filename);

private string custom_lpc_filename;
private object custom_lpc_object;

static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);
  if(clone) {
    MAPD->add_room_object(this_object());
  }
}

void destructed(int clone) {
  if(SYSTEM()) {
    room::destructed(clone);
    unq::destructed(clone);
    if(clone) {
      MAPD->remove_room_object(this_object());
    }
  }
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    room::upgraded(clone);
    unq::upgraded(clone);

    set_filename(custom_lpc_filename);
  }
}

void set_filename(string filename) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Can't call custom_room:set_filename unprivileged!");

  if(custom_lpc_filename && filename != custom_lpc_filename)
    destruct_object(custom_lpc_filename);

  custom_lpc_filename = filename;

  if(custom_lpc_filename) {
    compile_object(custom_lpc_filename);
    if(!custom_lpc_object) {
      custom_lpc_object = find_object(custom_lpc_filename);
    }
  }

  if(custom_lpc_object
     && function_object("dummy_func", custom_lpc_object) != CUSTOM_ROOM_PARENT)
    error("Invalid LPC Object in custom_room!  Must inherit from CR_PARENT!");
}

string to_unq_text(void) {
  if(SYSTEM() || COMMON() || GAME()) {
    return "~object{\n  obj_type{custom}\n"
      + to_unq_flags()
      + "\n  ~data{~filename{" + custom_lpc_filename + "}}\n}\n\n";
  }
  return nil;
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
      if((typeof(dtd_tag_val) != T_ARRAY)
	 || (sizeof(dtd_tag_val) != 2)
	 || (dtd_tag_val[0] != "filename"))
	error("Error in data section of UNQ for custom room obj!");

      set_filename(dtd_tag_val[1]);
    } else {
      from_dtd_tag(dtd_tag_name, dtd_tag_val);
    }
  }
}

/**************** Scripted Functions ********************************/

/* These functions call through to the custom_lpc_object to determine
   their behavior. */

