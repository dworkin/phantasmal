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

/* These numbers will be used to determine what a player can carry,
   and what objects can fit into what other objects.  Weight is
   in units of kilograms, and volume is in cubic decimeters
   (note 1 cu meter is 1000 sq decimeters).  Length represents the
   greatest extent of the longest axis, and is in units of
   centimeters.  The capacities are the largest values that are
   acceptable for objects put into this object.  Weight and
   volume are totalled among all objects put inside and compared to
   the capacity, while length is only compared to make sure it's
   no more than the capacity -- a quiver of arrows can accept a very
   large number of arrows, even though they're all of the maximum
   acceptable length. */
float weight, volume, length;
float weight_capacity, volume_capacity, length_capacity;


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


/*
 * Get and set functions for fields
 */

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

float get_weight(void) {
  if(weight < 0.0 && archetype)
    return archetype->get_weight();

  return weight < 0.0 ? 0.0 : weight;
}

float get_volume(void) {
  if(volume < 0.0 && archetype)
    return archetype->get_volume();

  return volume < 0.0 ? 0.0 : volume;
}

float get_length(void) {
  if(length < 0.0 && archetype)
    return archetype->get_length();

  return length < 0.0 ? 0.0 : length;
}

float get_weight_capacity(void) {
  if(weight_capacity < 0.0 && archetype)
    return archetype->get_weight_capacity();

  return weight_capacity;
}

float get_volume_capacity(void) {
  if(volume_capacity < 0.0 && archetype)
    return archetype->get_volume_capacity();

  return volume_capacity;
}

float get_length_capacity(void) {
  if(length_capacity < 0.0 && archetype)
    return archetype->get_length_capacity();

  return length_capacity;
}

void set_weight(float new_weight) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can set weights!");

  weight = new_weight;
}

void set_volume(float new_volume) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can set volumes!");

  volume = new_volume;
}

void set_length(float new_length) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can set lengths!");

  length = new_length;
}

void set_weight_capacity(float new_weight_capacity) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can set weight capacities!");

  weight_capacity = new_weight_capacity;
}

void set_volume_capacity(float new_volume_capacity) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can set volume capacities!");

  volume_capacity = new_volume_capacity;
}

void set_length_capacity(float new_length_capacity) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM code can set length capacities!");

  length_capacity = new_length_capacity;
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


/*
 * overloaded room notification functions
 */

/****** Functions dealing with entering and leaving a room ********/
/* return nil if the user can leave/enter, or a string indicating the
 * reason why they cannot.
 */

/* function which returns an appropriate error message if this object
 * isn't a container or isn't open
 */
static string is_open_cont(object user) {
  if (!is_container()) {
    if(!user) return "not container";
    return get_brief()->to_string(user) + " isn't a container!";
  }
  if (!is_open()) {
    if(!user) return "not open";
    return get_brief()->to_string(user) + " isn't open!";
  }
  return nil;
}


/* Note: Many functions are trivial here (always returning nil) but
 * can be overriden to control access into or out of a room 
 * Reason is printed out to the user if the user can't enter */

/* Note also: These functions can be used by the child, even if it
   overrides.  So it's easy for a child to add reasons why an
   object can't be put, entered, etc, but also easy for them to
   change the rules entirely. */

/* The can_XXX functions all take a user.  That user is necessary
   because to return the perceived reason a user can't do something,
   it's often necessary to know what that user can perceive. */

/* user is the user who will see the reason returned,
   leave_object is the body attempting to leave,
   dir is the direction. */
string can_leave(object user, object leave_object, int dir) {
  if(mobile)
    return "You can't leave a sentient being!"
      + "  In fact, you shouldn't even be here.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "not container";
      return get_brief()->to_string(user) + " isn't a container.";
    } else {
      return nil;
    }
  } else {
    return is_open_cont(user);
  }
}


/* user is the user who will see the reason returned,
   enter_object is the body attempting to enter,
   dir is the direction. */
string can_enter(object user, object enter_object, int dir) {
  if(mobile)
    return "You can't enter a sentient being!  Don't be silly.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "not container";
      return get_brief()->to_string(user) + " isn't a container.";
    } else {
      return nil;
    }
  } else {
    return is_open_cont(user);
  }
} 


/* leave_object is the body leaving, dir is the direction */
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

/* enter_object is the body entering, dir is the direction */
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

/*
  user is the user who will see the reason returned
  mover is the body of the mobile doing the moving,
  movee is the object being moved (one of this object's contained objects)
  new_env is the location that the object will shortly be in, which is
           contained by this object or contains this object.
*/
string can_remove(object user, object mover, object movee, object new_env) {
  return is_open_cont(user);
}


/* This function notifies us that an object has been removed from us.

   mover is the body of the mobile doing the moving
   movee is the object being removed from us
   env is the location it will shortly be moved to

   Note:  this function does *not* need to call remove_from_container
   or otherwise remove the object from itself.  That will be done
   separately.  This is just notification that the removal is
   going to occur.
*/
void remove(object mover, object movee, object new_env) {
}

/*
 * get functions are called in this object when this object is being
 * moved from its current room into new_room as part of a move operation
 */

/* This function is called to see whether this object may be taken.

  user is the user who will see the reason returned
  mover is the body of the mobile attempting to move this object
  new_env is where it will be moving it to
*/
string can_get(object user, object mover, object new_env) {
  if(mobile) {
    if(!user) return "sentient being";
    return get_brief()->to_string(user)
      + " is a sentient being!  You can't pick them up.";
  }
  return nil;
}


/* This function notifies us that somebody has gotten/moved this
   object.

   user is the user who will see the reason returned
   mover is the body of the mobile doing the moving
   new_env is where it is moving us to

   This function doesn't need to move the object from its parent
   to the new environment.  That'll be done separately.  This is
   just notification that that has happened.
*/
void get(object mover, object new_env) {
}


/* 
 * put functions are called when the movee object is being
 * moved from another object into this room.  The other
 * object can be the parent or a child.
 */

/*
  This function determines whether an object may be put
  somewhere.

  user is the user who will see the reason returned
  mover is the body of the one doing the moving
  movee is the object being moved
  old_env is the location of the object now containing the movee
*/

string can_put(object user, object mover, object movee, object old_env) {
  return is_open_cont(user);
}

/*
  This function notifies us that an object has been put.

  mover is the body of the one doing the moving
  movee is the object being moved
  old_env is the location of the object that just contained movee
*/
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
  
/* old style
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
  } */

  /* new style */
  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    exit_arr = exits[ctr];
    exit = exit_arr[1];

    if (exit->get_link() > 0) {
      dest = exit->get_destination();
      if(dest) {
        opp_dir = EXITD->opposite_direction(exit->get_direction());
        other_exit = dest->get_exit(opp_dir);
        if(!other_exit || other_exit->get_destination() != this_object()) {
          LOGD->write_syslog("Problem finding return exit!");
        } else {
          if(exit->get_number() < other_exit->get_number()) {
	    ret += exit->to_unq_text();
          }
	}
      } else {
        LOGD->write_syslog("Couldn't find destination!", LOG_WARNING);
      }
    } else {
      ret += exit->to_unq_text();
    }
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

  ret += "  ~weight{" + weight + "}\n";
  ret += "  ~volume{" + volume + "}\n";
  ret += "  ~length{" + length + "}\n";
  if(is_container()) {
    ret += "  ~weight_capacity{" + weight_capacity + "}\n";
    ret += "  ~volume_capacity{" + volume_capacity + "}\n";
    ret += "  ~length_capacity{" + length_capacity + "}\n";
  }

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
  /* newexit stuff */
  } else if(tag == "newexit") {
    EXITD->room_request_complex_exit(tr_num, value);
  } else if(tag == "weight") {
    weight = value;
  } else if(tag == "volume") {
    volume = value;
  } else if(tag == "length") {
    length = value;
  } else if(tag == "weight_capacity") {
    weight_capacity = value;
  } else if(tag == "volume_capacity") {
    volume_capacity = value;
  } else if(tag == "length_capacity") {
    length_capacity = value;
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
