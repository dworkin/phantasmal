#include <config.h>
#include <type.h>
#include <log.h>
#include <phrase.h>
#include <map.h>

#include <kernel/kernel.h>

/* room.c:

   A basic MUD room with standard trimmings
*/

inherit obj OBJECT;

mixed* exits;

private int  pending_location;
private int  pending_parent;
private int  pending_detail_of;
private int* pending_removed_details;

/* Flags */
/* The objflags field contains a set of boolean object flags */
#define OF_CONTAINER          1
#define OF_OPEN               2
#define OF_OPENABLE           8

private int objflags;


#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  obj::create(clone);
  if(clone) {
    exits = ({ });

    pending_parent = -1;
    pending_location = -1;
    pending_detail_of = -1;
    pending_removed_details = ({ });
  }
}

void destructed(int clone) {
  int index;
  mixed *objs;

  /* Remove contained objects, put them where this object used to
     be. */
  objs = objects_in_container();
  for(index = 0; index < sizeof(objs); index++) {
    remove_from_container(objs[index]);

    if(location)
      location->add_to_container(objs[index]);
  }

  /* Destruct all details */
  objs = get_immediate_details();
  if(!objs) objs = ({ }); /* Prevent error below */
  for(index = 0; index < sizeof(objs); index++) {
    remove_detail(objs[index]);
    EXITD->clear_all_exits(objs[index]);
    destruct_object(objs[index]);
  }

  if(detail_of)
    location->remove_detail(this_object());

  obj::destructed(clone);
}

void upgraded(varargs int clone) {
  obj::upgraded();
}

void set_number(int new_num) {
  if(previous_program() == MAPD) {
    tr_num = new_num;
  } else error("Only MAPD can set room numbers!");
}

void enum_room_mobiles(string cmd, object *except, mixed *args) {
  object *mobiles;
  int i;

  mobiles = mobiles_in_container();
  for (i = sizeof(mobiles); --i >= 0; ) {
    if (sizeof( ({ mobiles[i] }) & except ) == 0) {
      call_other(mobiles[i], cmd, args);
    }
  }
}


/*** Functions dealing with Exits ***/

void clear_exits(void) {
  if(previous_program() == EXITD)
    exits = ({ });
  else error("Only EXITD can clear exits!");
}

void add_exit(int dir, object exit) {
  if(previous_program() == EXITD) {
    exits = exits + ({ ({ dir, exit }) });
  } else error("Only EXITD can add exits!");
}

int num_exits(void) {
  return sizeof(exits);
}

object get_exit_num(int index) {
  if(index < 0) return nil;
  if(index >= sizeof(exits)) return nil;

  return exits[index][1];
}

object get_exit(int dir) {
  int ctr;

  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    if(exits[ctr][0] == dir)
      return exits[ctr][1];
  }

  return nil;
}

void remove_exit(object exit) {
  if(previous_program() == EXITD) {
    int ctr;

    for(ctr = 0; ctr < sizeof(exits); ctr++) {
      if(exits[ctr][1] == exit) {
	exits = exits[..ctr-1] + exits[ctr+1..];
	return;
      }
    }

    LOGD->write_syslog("Can't find exit to remove [" + sizeof(exits)
		       + " exits]!", LOG_ERR);
  } else error("Only EXITD can remove exits!");
}


/* 
 * flag overrides
 */

int is_container() {
  return objflags & OF_CONTAINER;
}

int is_open() {
  return objflags & OF_OPEN;
}

int is_openable() {
  return objflags & OF_OPENABLE;
}

private void set_flags(int flags, int value) {
  if(value) {
    objflags |= flags;
  } else {
    objflags &= ~flags;
  }
}

void set_container(int value) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as a container!");

  set_flags(OF_CONTAINER, value);
}

void set_open(int value) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as open!");

  set_flags(OF_OPEN, value);
}

void set_openable(int value) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can currently set an object as openable!");

  set_flags(OF_OPENABLE, value);
}


/****** Functions dealing with entering and leaving a room ********/
/* return nil if the user can leave/enter, or a string indicating the
 * reason why they cannot.
 */

/* Note: These functions are trivial here (always returning nil) but
 * can be overriden to control access into or out of a room 
 * Reason is printed out to the user if the user can't enter */
string can_leave(object leave_object, int dir) {
  return nil;
}

string can_enter(object enter_object, int dir) {
  return nil;
}

void leave(object leave_object, int dir) {
  object mob;

  mob = leave_object->get_mobile();
  
  /* 
   * notify all mobiles in the room that this person is leaving.
   * any user mobiles are then responsable for writing this to the 
   * user's terminal
   */
  enum_room_mobiles("hook_leave", ({ mob }), ({ leave_object, dir }) );
}

void enter(object enter_object, int dir) {
  object mob;

  mob = enter_object->get_mobile();

  enum_room_mobiles("hook_enter", ({ mob }), ({ enter_object, dir }) );
}

/****** Picking up/dropping functions *********/

/*
 * remove functions are called when the movee object is being
 * removed from this object.  The object can be being moved into
 * the parent or a child of this object.
 */

string can_remove(object mover, object movee, object new_env) {
  return nil;
}

void remove(object mover, object movee, object new_env) {
}

/*
 * get functions are called in this object when this object is being
 * moved from its current room into new_room as part of a move operation
 */

string can_get(object mover, object new_env) {
  if (get_detail_of())
    return "That's attached!  You can't get it.";

  return nil;
}

void get(object mover, object new_env) {
}

/* 
 * put functions are called when the movee object is being
 * moved from another object into this room.  The other
 * object can be the parent or a child.
 */

string can_put(object mover, object movee, object old_env) {
  return nil;
}

void put(object mover, object movee, object old_env) {
}

/********* UNQ serialization helper functions */

int get_pending_location(void) {
  return pending_location;
}

int get_pending_parent(void) {
  return pending_parent;
}

int get_pending_detail_of(void) {
  return pending_detail_of;
}

int* get_pending_removed_details(void) {
  return pending_removed_details;
}

void clear_pending(void) {
  pending_location = pending_parent = pending_detail_of = -1;
  pending_removed_details = ({ });
}

/* Include only exits that appear to have been created from this room
   so that they aren't doubled up when reloaded */
private string exits_to_unq(void) {
  object exit, dest, other_exit;
  mixed* exit_arr;
  int    ctr, opp_dir;
  string ret;
  object shortphr;

  ret = "";
  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    exit_arr = exits[ctr];
    exit = exit_arr[1];
    dest = exit->get_destination();
    if(dest) {
      opp_dir = EXITD->opposite_direction(exit->get_direction());
      other_exit = dest->get_exit(opp_dir);
      if(!other_exit || other_exit->get_destination() != this_object()) {
	LOGD->write_syslog("Problem finding return exit!");
      } else {
	if(exit->get_number() < other_exit->get_number()) {
	  shortphr = EXITD->get_short_for_dir(exit->get_direction());

	  ret += "  ~exit{"
	    + shortphr->get_content_by_lang(LANG_englishUS)
	    + ": #" + dest->get_number() + " "
	    + exit->get_number()
	    + " " + other_exit->get_number() + "}\n";
	}
      }
    } else
      LOGD->write_syslog("Couldn't find destination!", LOG_WARNING);
  }

  return ret;
}

/*
 * string to_unq_flags(void) 
 *
 * creates a string out of the object flags.
 */

string to_unq_flags(void) {
  string  ret, tmp_n, tmp_a;
  object *rem;
  int     locale, ctr;

  ret = "  ~number{" + tr_num + "}\n";
  if (get_detail_of()) {
    ret += "  ~detail{" + get_detail_of()->get_number() + "}\n";
  } else if (location) {
    ret += "  ~location{" + location->get_number() + "}\n";
  }
  ret += "  ~bdesc{" + bdesc->to_unq_text() + "}\n";
  ret += "  ~gdesc{" + gdesc->to_unq_text() + "}\n";
  ret += "  ~ldesc{" + ldesc->to_unq_text() + "}\n";
  if(edesc) {
    ret += "  ~edesc{" + edesc->to_unq_text() + "}\n";
  }
  ret += "  ~flags{" + objflags + "}\n";

  if(archetype) {
    ret += "  ~parent{" + archetype->get_number() + "}\n";
  }
  ret += "  ~article{" + desc_article + "}\n";

  /* Skip debug locale */
  tmp_n = tmp_a = "";
  for(locale = 1; locale < sizeof(nouns); locale++) {
    if(sizeof(nouns[locale])) {
      tmp_n += "~" + PHRASED->locale_name_for_language(locale) + "{"
	+ implode(nouns[locale], ",") + "}";
    }
    if(sizeof(adjectives[locale])) {
      tmp_a += "~" + PHRASED->locale_name_for_language(locale) + "{"
	+ implode(adjectives[locale], ",") + "}";
    }
  }

  /* The double-braces are intentional -- this uses the efficient
     method of specifying nouns and adjectives rather than the human-
     friendly one. */
  ret += "  ~nouns{{" + tmp_n + "}}\n";
  ret += "  ~adjectives{{" + tmp_a + "}}\n";

  ret += exits_to_unq();

  if(get_removed_details()) {
    rem = get_removed_details();

    ret += "  ~removed_details{";
    for(ctr = 0; ctr < sizeof(rem) - 1; ctr++) {
      ret += rem[ctr]->get_number() + ", ";
    }
    /* Add final element, no comma */
    if(sizeof(rem))
      ret += rem[sizeof(rem) - 1]->get_number();

    ret += "}\n";
  }

  return ret;
}

/*
 * void from_dtd_flags(mixed *unq)
 *
 * loads data from the unq parsed with a room-derived dtd.
 */

void from_dtd_tag(string tag, mixed value) {
  int ctr, ctr2;

  if(tag == "number")
    tr_num = value;

  else if(tag == "detail") {
    if(pending_location > -1) {
      LOGD->write_syslog("Detail specified despite pending location!",
			 LOG_ERR);
      LOGD->write_syslog("Obj #" + tr_num + ", detail field: "
			 + value + ", existing location/detail: "
			 + pending_location, LOG_ERR);
      error("Error loading object #" + tr_num + "!  Check logfile.");
    }
    pending_detail_of = value;
    pending_location = value;

  } else if(tag == "location") {
    if(pending_location > -1) {
      LOGD->write_syslog("Location specified despite pending location!",
			 LOG_ERR);
      LOGD->write_syslog("Obj #" + tr_num + ", new location: "
			 + value + ", existing location/detail: "
			 + pending_location, LOG_ERR);
      error("Error loading object #" + tr_num + "!  Check logfile.");
    }
    pending_location = value;

  } else if(tag == "bdesc")
    set_brief(value);
  else if(tag == "gdesc")
    set_glance(value);
  else if(tag == "ldesc")
    set_look(value);
  else if(tag == "edesc")
    set_examine(value);
  else if(tag == "article")
    desc_article = value;
  else if(tag == "flags")
    objflags = value;
  else if(tag == "parent")
    pending_parent = value;
  else if(tag == "nouns") {
    for(ctr2 = 0; ctr2 < sizeof(value); ctr2++) {
      add_noun(value[ctr2]);
    }
  } else if(tag == "adjectives") {
    for(ctr2 = 0; ctr2 < sizeof(value); ctr2++) {
      add_adjective(value[ctr2]);
    }
  } else if(tag == "exit") {
    string dirname;
    int    roomnum, dir, exitnum1, exitnum2;
    
    value = STRINGD->trim_whitespace(value);
    if(sscanf(value, "%s: #%d %d %d", dirname, roomnum,
	      exitnum1, exitnum2) == 4) {
      
      dir = EXITD->direction_by_string(dirname);
      if(dir == -1)
	error("Can't find direction for dirname " + dirname);
      
      if(tr_num <= 0)
	error("Can't yet request an exit from an unnumbered room!");
      
      EXITD->room_request_simple_exit(tr_num, roomnum, dir,
				      exitnum1, exitnum2);
    } else {
      error("Can't parse as exit desc: '" + value + "'");
    }
  } else if(tag == "removed_details") {
    if(typeof(value) == T_INT) {
      pending_removed_details = ({ value });
    } else if(typeof(value) == T_ARRAY) {
      pending_removed_details = value;
    } else
      error("Unreasonable type for removed_details!");
  } else { 
    error("Don't recognize tag " + tag + " in function from_dtd_tag()");
  }
}
