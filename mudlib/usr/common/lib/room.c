#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/map.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

/* room.c:

   A basic MUD room or object with standard trimmings.  This includes
   portables and containers, though not exits.
*/

inherit obj OBJECT;

private mixed* exits;

private int  pending_location;
private int* pending_parents;
private int  pending_detail_of;
private int* pending_removed_details;
private object* pending_removed_nouns;
private object* pending_removed_adjectives;

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
private float weight, volume, length;
private float weight_capacity, volume_capacity, length_capacity;

/* These are the total current amount of weight and volume
   being held in the object. */
private float current_weight, current_volume;


#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  obj::create(clone);
  if(clone) {
    exits = ({ });

    current_weight = 0.0;
    current_volume = 0.0;

    pending_parents = nil;
    pending_location = -1;
    pending_detail_of = -1;
    pending_removed_details = ({ });
    pending_removed_nouns = ({ });
    pending_removed_adjectives = ({ });
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

    if(obj::get_location())
      obj::get_location()->add_to_container(objs[index]);
  }

  /* Destruct all details */
  objs = get_immediate_details();
  if(!objs) objs = ({ }); /* Prevent error below */
  for(index = 0; index < sizeof(objs); index++) {
    remove_detail(objs[index]);
    EXITD->clear_all_exits(objs[index]);
    destruct_object(objs[index]);
  }

  if(obj::get_detail_of())
    obj::get_location()->remove_detail(this_object());

  if(obj::get_location()) {
    LOGD->write_syslog("Destructing a ROOM without removing it!",
		       LOG_WARN);
  }

  obj::destructed(clone);
}

void upgraded(varargs int clone) {
  /* TODO:  ROOM should recalculate weights and volumes when
     upgraded.  @stat might want to check the object as well. */

  obj::upgraded();
}


/*
 * Get and set functions for fields
 */

void enum_room_mobiles(string cmd, object *except, mixed args...) {
  object *mobiles;
  int i;

  mobiles = mobiles_in_container();
  for (i = sizeof(mobiles); --i >= 0; ) {
    if (sizeof( ({ mobiles[i] }) & except ) == 0) {
      call_other(mobiles[i], cmd, args...);
    }
  }
}

float get_weight(void) {
  if(weight < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_weight();

  return weight < 0.0 ? 0.0 : weight;
}

float get_volume(void) {
  if(volume < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_volume();

  return volume < 0.0 ? 0.0 : volume;
}

float get_length(void) {
  if(length < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_length();

  return length < 0.0 ? 0.0 : length;
}

float get_weight_capacity(void) {
  if(weight_capacity < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_weight_capacity();

  return weight_capacity;
}

float get_volume_capacity(void) {
  if(volume_capacity < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_volume_capacity();

  return volume_capacity;
}

float get_length_capacity(void) {
  if(length_capacity < 0.0 && sizeof(obj::get_archetypes()))
    return obj::get_archetypes()[0]->get_length_capacity();

  return length_capacity;
}

float get_current_weight(void) {
  return current_weight;
}

float get_current_volume(void) {
  return current_volume;
}

void set_weight(float new_weight) {
  object loc;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can set weights!");

  /* Remove from container and add back -- that way the weight
     will be correct */
  loc = obj::get_location();
  if(loc && !obj::get_detail_of()) {
    loc->remove_from_container(this_object());
  } else loc = nil;

  weight = new_weight;

  if(loc)
    loc->add_to_container(this_object());
}

void set_volume(float new_volume) {
  object loc;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can set volumes!");

  /* Remove from container and add back -- that way the weight
     will be correct */
  loc = obj::get_location();
  if(loc && !obj::get_detail_of()) {
    loc->remove_from_container(this_object());
  } else loc = nil;

  volume = new_volume;

  if(loc)
    loc->add_to_container(this_object());
}

void set_length(float new_length) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can set lengths!");

  length = new_length;
}

void set_weight_capacity(float new_weight_capacity) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can set weight capacities!");

  weight_capacity = new_weight_capacity;
}

void set_volume_capacity(float new_volume_capacity) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can set volume capacities!");

  volume_capacity = new_volume_capacity;
}

void set_length_capacity(float new_length_capacity) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can set length capacities!");

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
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can currently set an object as a container!");

  set_flags(OF_CONTAINER, value);
}

void set_open(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can currently set an object as open!");

  set_flags(OF_OPEN, value);
}

void set_openable(int value) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only authorized code can currently set an object as openable!");

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
  if(obj::get_mobile())
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
  string reason;
  object body;

  if(obj::get_mobile())
    return "You can't enter a sentient being!  Don't be silly.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "not container";
      return get_brief()->to_string(user) + " isn't a container.";
    } else {
      return nil;
    }
  } else {
    reason = is_open_cont(user);
    if(reason)
      return reason;
  }

  if(enter_object
     && (current_weight + enter_object->get_weight() > weight_capacity)) {
    if(!user)
      return "too full";
    else
      return get_brief()->to_string(user) + " is too full!";
  }

  if(enter_object
     && (current_volume + enter_object->get_volume() > volume_capacity)) {
    if(!user)
      return "too full";
    else
      return get_brief()->to_string(user) + " is too full!";
  }

  return nil;
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
  enum_room_mobiles("hook_leave", ({ mob }), leave_object, dir );
}

/* enter_object is the body entering, dir is the direction */
void enter(object enter_object, int dir) {
  object mob;

  mob = enter_object->get_mobile();

  enum_room_mobiles("hook_enter", ({ mob }), enter_object, dir );
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
  if(obj::get_mobile()) {
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
  string reason;

  reason = is_open_cont(user);
  if(reason)
    return reason;

  if(movee && (current_weight + movee->get_weight() > weight_capacity)) {
    if(!user)
      return "too full";
    else
      return get_brief()->to_string(user) + " is too full!";
  }

  if(movee && (current_volume + movee->get_volume() > volume_capacity)) {
    if(!user)
      return "too full";
    else
      return get_brief()->to_string(user) + " is too full!";
  }

  if(movee && (movee->get_length() > length_capacity)) {
    if(!user)
      return "not long enough";
    else
      return get_brief()->to_string(user)
	+ " can't hold something that long!";
  }
}

/*
  This function notifies us that an object has been put.

  mover is the body of the one doing the moving
  movee is the object being moved
  old_env is the location of the object that just contained movee
*/
void put(object mover, object movee, object old_env) {
}


/********* Overrides of OBJECT functions for containers */

/* Note:  add_to_container calls append_to_container, so we don't
   need to explicitly override it.  If we did, we'd get double-count
   on weight and volume added that way */

void append_to_container(object obj) {
  float obj_weight, obj_volume;

  obj_weight = obj->get_weight();
  if(obj_weight >= 0.0)
    current_weight += obj_weight;
  else
    LOGD->write_syslog("Negative weight in append_to_container!",
		       LOG_WARN);

  obj_volume = obj->get_volume();
  if(obj_volume >= 0.0)
    current_volume += obj_volume;
  else
    LOGD->write_syslog("Negative volume in append_to_container!",
		       LOG_WARN);

  obj::append_to_container(obj);
}


void prepend_to_container(object obj) {
  float obj_weight, obj_volume;

  obj_weight = obj->get_weight();
  if(obj_weight >= 0.0)
    current_weight += obj_weight;
  else
    LOGD->write_syslog("Negative weight in prepend_to_container!",
		       LOG_WARN);

  obj_volume = obj->get_volume();
  if(obj_volume >= 0.0)
    current_volume += obj_volume;
  else
    LOGD->write_syslog("Negative volume in prepend_to_container!",
		       LOG_WARN);

  obj::prepend_to_container(obj);
}

void remove_from_container(object obj) {
  float obj_weight, obj_volume;

  obj_weight = obj->get_weight();
  if(obj_weight >= 0.0)
    current_weight -= obj_weight;
  else
    LOGD->write_syslog("Negative weight in remove_from_container!",
		       LOG_WARN);

  obj_volume = obj->get_volume();
  if(obj_volume >= 0.0)
    current_volume -= obj_volume;
  else
    LOGD->write_syslog("Negative volume in remove_from_container!",
		       LOG_WARN);

  obj::remove_from_container(obj);
}


/******** Functions to manage pending fields ***********/

int get_pending_location(void) {
  return pending_location;
}

/* Can't override set_location, we don't have the necessary
   privilege to call it! */

int* get_pending_parents(void) {
  return pending_parents;
}

void set_archetypes(object* new_arch) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only SYSTEM and COMMON objects may set archetypes!");

  ::set_archetypes(new_arch);
  pending_parents = nil;
}

int get_pending_detail_of(void) {
  return pending_detail_of;
}

int* get_pending_removed_details(void) {
  return pending_removed_details;
}

object* get_pending_removed_nouns(void) {
  return pending_removed_nouns;
}

object* get_pending_removed_adjectives(void) {
  return pending_removed_adjectives;
}

void set_removed_details(object *new_removed_details) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only SYSTEM or COMMON objects can set removed_details!");

  pending_removed_details = ({ });
  ::set_removed_details(new_removed_details);
}

void clear_pending(void) {
  pending_location = pending_detail_of = -1;
  pending_removed_details = ({ });
  pending_parents = ({ });
  pending_removed_nouns = ({ });
  pending_removed_adjectives = ({ });
}


/********* UNQ serialization helper functions */

/* Include only exits that appear to have been created from this room
   so that they aren't doubled up when reloaded */
private string exits_to_unq(void) {
  object exit, dest, other_exit;
  mixed* exit_arr;
  int    ctr, opp_dir;
  string ret;
  object shortphr;

  ret = "";

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
          LOGD->write_syslog("Problem finding return exit!", LOG_WARN);
        } else {
/*          if(exit->get_number() < other_exit->get_number()) {  */
	    ret += exit->to_unq_text();
/*          } */
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


/* This function serializes tags from TagD so that they can be written
   into the object file. */
private string all_tags_to_unq(void) {
  mixed *all_tags;
  string ret;
  int    ctr;

  all_tags = TAGD->object_all_tags(this_object());
  ret = "";

  for(ctr = 0; ctr < sizeof(all_tags); ctr += 2) {
    ret += "~" + all_tags[ctr] + "{";
    switch(TAGD->object_tag_type(all_tags[ctr])) {
    case T_INT:
    case T_FLOAT:
      ret += all_tags[ctr + 1];
      break;

    case T_STRING:
      ret += STRINGD->unq_escape(all_tags[ctr + 1]);
      break;

    case T_OBJECT:
    case T_ARRAY:
    case T_MAPPING:
    default:
      error("Can't output that tag type yet!");
    }
    ret += "}\n        ";
  }

  return ret;
}


/* This is used to take a string** array, indexed first by locale
   and then by individual item, into a phrase double-array which
   can be parsed as UNQ. */
private string serialize_wordlist(string **wlist) {
  string tmp;
  int    locale;

  /* Skip debug locale */
  tmp = "";
  for(locale = 1; locale < sizeof(wlist); locale++) {
    if(wlist[locale] && sizeof(wlist[locale])) {
      tmp += "~" + PHRASED->locale_name_for_language(locale) + "{"
	+ implode(wlist[locale], ", ") + "}";
    }
  }

  return tmp;
}

/* This is used to serialize a list of numbers or objects into
   a comma-separated list of integers. */
private string serialize_list(mixed *list) {
  string *str_list;
  int     ctr;

  str_list = ({ });
  for(ctr = 0; ctr < sizeof(list); ctr++) {
    switch(typeof(list[ctr])) {
    case T_INT:
      str_list += ({ "" + list[ctr] });
      break;
    case T_OBJECT:
      str_list += ({ list[ctr]->get_number() + "" });
      break;
    case T_STRING:
      str_list += ({ list[ctr] });
      break;
    default:
      error("Error in stringifying list -- unacceptable object "
	    + STRINGD->mixed_sprint(list[ctr]));
    }
  }

  return implode(str_list, ", ");
}

/*
 * string to_unq_flags(void)
 *
 * creates a string out of the object flags.
 */

string to_unq_flags(void) {
  string  ret;
  object *rem, *arch;
  int     locale, ctr;

  ret = "  ~number{" + tr_num + "}\n";
  if (get_detail_of()) {
    ret += "  ~detail{" + get_detail_of()->get_number() + "}\n";
  } else if (obj::get_location()) {
    ret += "  ~location{" + obj::get_location()->get_number() + "}\n";
  }
  ret += "  ~bdesc{" + bdesc->to_unq_text() + "}\n";
  /* ret += "  ~gdesc{" + gdesc->to_unq_text() + "}\n"; */
  ret += "  ~ldesc{" + ldesc->to_unq_text() + "}\n";
  if(edesc) {
    ret += "  ~edesc{" + edesc->to_unq_text() + "}\n";
  }
  ret += "  ~flags{" + objflags + "}\n";

  arch = obj::get_archetypes();
  if(arch && sizeof(arch)) {
    ret += "  ~parent{" + serialize_list(arch) + "}\n";
  }

  /* The double-braces are intentional -- this uses the efficient
     method of specifying nouns and adjectives rather than the human-
     friendly one.  Both are parseable, naturally. */
  ret += "  ~nouns{{" + serialize_wordlist(get_immediate_nouns()) + "}}\n";
  ret += "  ~adjectives{{" + serialize_wordlist(get_immediate_adjectives())
    + "}}\n";

  arch = get_archetypes();
  if(arch && sizeof(arch)) {
    ret += "  ~rem_nouns{{" + serialize_wordlist(removed_nouns) + "}}\n";
    ret += "  ~rem_adjectives{{" + serialize_wordlist(removed_adjectives)
      + "}}\n";
  }

  ret += exits_to_unq();

  ret += "  ~weight{" + weight + "}\n";
  ret += "  ~volume{" + volume + "}\n";
  ret += "  ~length{" + length + "}\n";
  if(is_container()) {
    ret += "  ~weight_capacity{" + weight_capacity + "}\n";
    ret += "  ~volume_capacity{" + volume_capacity + "}\n";
    ret += "  ~length_capacity{" + length_capacity + "}\n";
  }

  rem = get_removed_details();
  if(rem && sizeof(rem)) {
    ret += "  ~removed_details{" + serialize_list(rem) + "}\n";
  }

  ret += "  ~tags{" + all_tags_to_unq() + "}\n";

  return ret;
}

private void parse_all_tags(mixed* value) {
  int    ctr, type, do_sscanf;
  string format_code;
  mixed  new_val;

  value = UNQ_PARSER->trim_empty_tags(value);

  for(ctr = 0; ctr < sizeof(value); ctr += 2) {
    value[ctr] = STRINGD->trim_whitespace(value[ctr]);
    type = TAGD->object_tag_type(value[ctr]);

    switch(type) {
    case -1:
      error("No such tag as '" + STRINGD->mixed_sprint(value[ctr])
	    + "' defined in TagD!");

    case T_INT:
      do_sscanf = 1;
      format_code = "%d";
      break;

    case T_FLOAT:
      do_sscanf = 1;
      format_code = "%f";
      break;

    default:
      error("Can't parse tags of type " + type + " yet!");
    }

    if(do_sscanf) {
      if(typeof(value[ctr + 1]) != T_STRING)
	error("Internal error:  Can't read tag out of non-string value!");

      value[ctr + 1] = STRINGD->trim_whitespace(value[ctr + 1]);

      sscanf(value[ctr + 1], format_code, new_val);
      TAGD->set_object_tag(this_object(), value[ctr], new_val);
    } else {
      /* Nothing yet, if not sscanf */
    }
  } /* END: for(ctr = 0; ctr < sizeof(value); ctr += 2) */
}

/*
 * void from_dtd_tag(string tag, mixed value)
 *
 * Grabs data from one field of the DTD-parsed UNQ.  This function
 * is so that child classes can easily add new fields, but still
 * have the parent parse the fields that are known to it.
 */

void from_dtd_tag(string tag, mixed value) {
  int ctr, ctr2;

  if(tag == "number")
    tr_num = value;

  else if(tag == "obj_type") {
    /* Nothing...  Already handled */

  } else if(tag == "detail") {
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
    /* set_glance(value) */;
  else if(tag == "ldesc")
    set_look(value);
  else if(tag == "edesc")
    set_examine(value);
  else if(tag == "article")
    /* desc_article = value */;
  else if(tag == "flags")
    objflags = value;
  else if(tag == "parent")
    pending_parents = ({ value });
  else if(tag == "nouns") {
    for(ctr2 = 0; ctr2 < sizeof(value); ctr2++) {
      add_noun(value[ctr2]);
    }
  } else if(tag == "adjectives") {
    for(ctr2 = 0; ctr2 < sizeof(value); ctr2++) {
      add_adjective(value[ctr2]);
    }
  } else if(tag == "rem_nouns") {
    for(ctr2 = 0; ctr2 < sizeof(value); ctr2++) {
      pending_removed_nouns += ({ value[ctr2] });
    }
  } else if(tag == "rem_adjectives") {
    for(ctr2 = 0; ctr2 < sizeof(value); ctr2++) {
      pending_removed_adjectives += ({ value[ctr2] });
    }
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
  } else if(tag == "tags") {
    /* Fill in tags array for this object */
    parse_all_tags(value);

  } else {
    error("Don't recognize tag " + tag + " in function from_dtd_tag()");
  }
}
