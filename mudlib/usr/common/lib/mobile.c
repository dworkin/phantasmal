#include <map.h>
#include <config.h>
#include <kernel/kernel.h>
#include <phrase.h>

/* Mobile: structure for a sentient, not-necessarily-player critter's
   mind.  The mobile will be attached to a body under any normal
   circumstances.
*/

inherit unq UNQABLE;

/*
 * cached vars
 */

static object body;     /* The mobile's body -- an OBJECT of some type */
static object location;
static int    number;

/*
 * Note:  inherits create() and upgraded() from UNQABLE
 */

/*
 * System functions
 *
 * Functions used to deal with the mobile elsewhere in the MUD
 */

void assign_body(object new_body) {
  if(!SYSTEM()) {
    error("Only SYSTEM objects can assign a mobile a new body!");
  }

  if(body) {
    body->set_mobile(nil);
    body = nil;
  }

  if(new_body) {
    new_body->set_mobile(this_object());
  }

  body = new_body;
  location = body->get_location();
}

object get_body(void) {
  return body;
}

object get_user(void) {
  /* return nil, the default mobile doesn't have a user */
  return nil;
}

void set_user(object new_user) {
  error("Can't set the user of this kind of mobile");
}

int get_number(void) {
  return number;
}

void set_number(int new_num) {
  if(previous_program() != MOBILED) {
    error("Only MOBILED can set mobile numbers!");
  }
  number = new_num;
}

void notify_moved(object obj) {
  /* Since only privileged code can reassign a body, this is fine
     for anybody to call -- it's just a cache update. */
  location = body->get_location();
}

/*
 * Action functions
 * 
 * Functions called by the user object or inherited objects to do
 * stuff through mobile's body
 */

/*
 * Say
 *
 * Tells something to everyone in the room
 */

nomask void say(string msg) {
  location->enum_room_mobiles("hook_say", ({ this_object() }),
			      ({ body, msg }) );
  
  if (get_user()) {
    get_user()->message("You say: " + msg + "\r\n");
  }
}

/*
 * emote()
 *
 * Sends an emote to everyone in the room.  Emotes may be completely replaced
 * by souls eventually. (Keith Dunwoody's personal preference).
 */

nomask void emote(string str) {
  /* For an emote, show the user the same message everybody else sees.
     For instance "Bob sits still." rather than "You sits still.". */
  location->enum_room_mobiles("hook_emote", ({ }),
			      ({ body, str  }));
}

/*
 * social()
 *
 * Does a social/soul action.  This will be visible to everyone in the
 * room and may appear different to the (optional) target.  The "target"
 * parameter should point to the target's body.
 */

nomask int social(string verb, object target) {
  location->enum_room_mobiles("hook_social", ({ }),
			      ({ body, target, verb  }));
  return 0;
}

/*
 * void whisper()
 *
 * object to: body of the object to whisper to.
 *
 * Whisper to someone or something.  They must be in the same location as you.
 */
nomask int whisper(object to, string str) {
  object mob;

  if (to->get_location() != location) {
    return 0;
  }

  mob = to->get_mobile();
  if (mob == nil) {
    return 0;
  }
  mob->hook_whisper(body, str);
  if (get_user()) {
    get_user()->message("You whisper to ");
    get_user()->send_phrase(to->get_brief());
    get_user()->message(": " + str + "\r\n");
  }
  location->enum_room_mobiles("hook_whisper_other",
			      ({ this_object(), mob }), ({ body, to }) );
  
  return 1;
}

/* 
 * ask()
 * 
 * Function to ask something.  Everyone in the same room as you can hear your
 * ask.
 */

nomask int ask(object to, string str) {
  object user;
  object mob;

  user = get_user();

  if (to->get_location() != location) {
    return 0;
  }

  if (to == nil) {
    if (user) {
      user->message("You ask: " + str + "\r\n");
    }

    location->enum_room_mobiles("hook_ask_other", ({ this_object() }),
				({ body, nil, str }) );
  } else {
    mob = to->get_mobile();
    if (mob == nil) {
      return 0;
    }
    mob->hook_ask(body, str);
    if (user) {
      user->message("You ask ");
      user->send_phrase(to->get_brief());
      user->message(": " + str + "\r\n");
    }

    location->enum_room_mobiles("hook_ask_other", ({ this_object(), mob }),
				({ body, to, str }) );
  }
  
  return 1;
}

/*
 * path_place()
 *
 * moves the object along the path, removing the object from all objects
 * in rem_path, and adding it to all objects in add_path, in order.
 * rem_path & add_path must already be in the proper order -- no checks
 * are performed.
 */
private atomic void path_place(object *rem_path, object *add_path, object obj) {
  int i;
  object env;
  string reason;

  /* assume the full move can be performed.  If it can't we'll throw an error,
     which will cause this function to be completely undone when the error
     passes out of the function.  Hurray for atomics!
  */

  for (i = 0; i < sizeof(rem_path); ++i) {
    env = rem_path[i]->get_location();
    if ((reason = rem_path[i]->can_remove(body, obj, env)) ||
	(reason = obj->can_get(body, env)) ||
	(reason = env->can_put(body, obj, rem_path[i]))) {
      error("$" + reason);
    } else {
      /* call function in object for removing from the current room */
      obj->get(body, env);
      rem_path[i]->remove(body, obj, env);
      rem_path[i]->remove_from_container(obj);
      env->add_to_container(obj);
      env->put(body, obj, rem_path[i]);
    }
  }

  /* now add this object to the objects in the add path in order */
  for (i = 0; i < sizeof(add_path); ++i) {
    env = add_path[i]->get_location();
    if ((reason = add_path[i]->can_put(body, obj, env)) ||
	(reason = obj->can_get(body, add_path[i])) ||
	(reason = env->can_remove(body, obj, add_path[i]))) {
      error("$" + reason);
    } else {
      obj->get(body, add_path[i]);
      env->remove(body, obj, add_path[i]);
      env->remove_from_container(obj);
      add_path[i]->add_to_container(obj);
      add_path[i]->put(body, obj, env);
    }
  }
}

/* 
 * place()
 *
 * move the object obj from its current position into the object to.
 * obj and to must both be descendents (but not necessarily children)
 * of the current room.
 *
 * returns nil on success, a reason for the failure on failure
 */
nomask string place(object obj, object to) {
  object cur_loc;
  object *rem_tree, *add_tree;
  int common;
  string err;    
  object user;
  int i;

  /* find out how many rooms this object can be removed from, ending 
   * when we find the mobile's location.
   */

  rem_tree = ({ });
  add_tree = ({ });

  cur_loc = obj->get_location();
  while(cur_loc != location) {
    if (cur_loc == nil) {
      /* the object to move isn't a descendent of the mobile's current
       * location
       */
      if (get_user()) {
	err = obj->get_brief()->to_string(get_user());
      } else {
	err = obj->get_brief()->get_content_by_lang(LANG_englishUS);
      }
      err += " is not in this room";
      return err;
    }

    rem_tree += ({ cur_loc });
    cur_loc = cur_loc->get_location();
  }

  /* do the same thing for moving the object into the container, except
   * include the container */

  cur_loc = to;
  while (cur_loc != location) {
    if (cur_loc == nil) {
      /* the place to move to is not a descendent of the mob's location
       * so return an error
       */
      if (get_user()) {
	err = to->get_brief()->to_string(get_user()); + " is not in this room";
      } else {
	err = to->get_brief()->get_content_by_lang(LANG_englishUS);
      }
      err += " is not in this room";

      return err;
    }

    add_tree += ({ cur_loc });
    cur_loc = cur_loc->get_location();
  }

  /* remove all common elements from the ends of the remove & add lists */

  common = sizeof(add_tree & rem_tree);

  if (common != 0) {
    add_tree = add_tree[..(sizeof(add_tree)-common-1)];
    rem_tree = rem_tree[..(sizeof(rem_tree)-common-1)];
  }

  err = catch(path_place(rem_tree, add_tree, obj));

  if (err) {
    /* non-serious errors will be prefixed with a '$' -- in which case we
     * strip the leading $, and return the error.
     */
    if (err[0] == '$') {
      return err[1..];
    } else {
      /* serious error -- re-throw */
      error(err);
    }
  }

  return nil;
}

/*
 * string open()
 *
 * have the mobile attempt to open the given object.
 *
 * return nil on success, or a string indicating the reason for
 * the failure on failure.  (replace this by a phrase later?)
 */
nomask string open(object obj) {
  object link_exit;

  if(!obj->is_openable() || (!obj->is_container() && obj->get_type() != "EXIT")) {
    return "That can't be opened!";
  }

  if(obj->is_open()) {
    return "That's already open.";
  }

  if(obj->is_locked()) {
    return "That appears to be locked.";
  }

  if (obj->get_type()=="EXIT") {
    obj->set_open(1);
    if (obj->get_link()!=-1) {
      link_exit = EXITD->get_exit_by_num(obj->get_link());
      link_exit->set_open(1);
    }
  } else {
    obj->set_open(1);
  }

  if(obj->get_type() != "EXIT"
     && obj->get_location() != body
     && obj->get_location() != location) {
    return "You can't reach that from here.";
  }

  return nil;
}

/*
 * string close()
 *
 * have the mobile attempt to close the given object.
 *
 * return nil on success, or a string indicating the reason for
 * the failure on failure.  (replace this by a phrase later?)
 */
nomask string close(object obj) {
  object link_exit;

  if(!obj->is_openable() || (!obj->is_container() && obj->get_type() != "EXIT")) {
    return "That can't be closed!";
  }

  if(!obj->is_open()) {
    return "That's already closed.";
  }

  if(obj->get_location() != location
     && obj->get_location() != body
     && obj->get_type() != "EXIT") {
    return "You can't reach that from here.";
  }

  if (obj->get_type()=="EXIT") {
    obj->set_open(0);
    if (obj->get_link()!=-1) {
      link_exit = EXITD->get_exit_by_num(obj->get_link());
      link_exit->set_open(0);
    }
  } else {
    obj->set_open(0);
  }

  return nil;
}


/*
 * string move()
 *
 * move's the mobile's body through the given exit.
 *
 * return nil on success, or a string indicating the reason for
 * the failure on failure.  (replace this by a phrase later?)
 */

nomask string move(int dir) {
  object dest;
  object exit;
  string reason;

  exit = location->get_exit(dir);
  if (!exit) {
    return "There's no exit in that direction!";
  }

  dest = exit->get_destination();
  if (!dest) {
    return "That exit doesn't seem to lead anywhere!";
  }

  /* NB.  I do want a = (not == ), as in other places like this*/
  if (reason = location->can_leave(body, dir)) {
    return reason;
  }

  if (reason = exit->can_pass(body)) {
    return reason;
  }

  if (reason = dest->can_enter(body, EXITD->opposite_direction(dir))) {
    return reason;
  }
  
/*  if (!exit->is_open()) {
    return "That way is closed.\n\r";
  } */

  location->leave(body, dir);
  location->remove_from_container(body);
  exit->pass(body);
  dest->add_to_container(body);
  dest->enter(body, EXITD->opposite_direction(dir));

  return nil;
}


/*
 * string teleport()
 *
 * teleport's the mobile's body to the given destination.
 * 
 * parameters:
 * force -- if true, forces the teleport to always succeed.
 *
 * return -- the reason why the teleport didn't succeed, or nil on success
 */

nomask string teleport(object dest, int force) {
  string reason;

  if (!force) {
    if (location) {
      if (reason = location->can_leave(body, DIR_TELEPORT)) {
	return reason;
      }
    }
    
    if (reason = dest->can_enter(body, DIR_TELEPORT)) {
      return reason;
    }
  }

  if (location) {
    location->leave(body, DIR_TELEPORT);
    location->remove_from_container(body);
  }

  dest->add_to_container(body);
  dest->enter(body, DIR_TELEPORT);

  return nil;
}


/*
 * Hook functions
 *
 * Functions which can be overridden in a derived class to respond to external
 * events.  In the standard mobile object, these have empty definitions
 *
 * Don't bother calling these base functions, as they will never do anyting.
 * they're here as documentation, nothing else.
 */

/*
 * first arg: body who said
 * second arg: what they said
 */

void hook_say(mixed *args) {
}

/*
 * first arg: body who said
 * second arg: what they said
 */

void hook_emote(mixed *args) {
}

/*
 * first arg (object): body who acted out the social
 * second arg (object): target body or nil
 * third arg (string): the name of the social
 */

void hook_social(mixed *args) {
}

/*
 * first arg: body that whispered
 * second arg: what they whispered
 */

void hook_whisper(mixed *args) {
}

/*
 * first arg: body that whispered
 * second arg: who they whispered to
 */

void hook_whisper_other(mixed *args) {
}

/*
 * first arg: body that asked
 * second arg: what they asked
 */

void hook_ask(mixed *args) {
}

/*
 * first arg: body that asked
 * second arg: body they asked
 * third arg: what they asked
 */

void hook_ask_other(mixed *args) {
}

/*
 * first arg: the body that left
 * second arg: the direction they left
 */

void hook_leave(mixed *args) {
}

/*
 * first arg: the body that entered
 * second arg: the direction they entered from
 */

void hook_enter(mixed *args) {
}


/*
 * UNQ functions
 */

string to_unq_text(void) {
  string ret;
  int    bodynum;

  if(body) {
    bodynum = body->get_number();
  } else {
    bodynum = -1;
  }

  ret  = "~mobile{\n";
  ret += "  ~type{" + this_object()->get_type() + "}\n";
  ret += "  ~name{"
    + body->get_brief()->get_content_by_lang(LANG_englishUS)
    + "}\n";
  ret += "  ~number{" + number + "}\n";
  ret += "  ~body{" + bodynum + "}\n";
  if(function_object("mobile_unq_fields", this_object())) {
    ret += "  ~data{\n";
    ret += this_object()->mobile_unq_fields();
    ret += "  }\n";
  }
  ret += "}\n\n";

  return ret;
}

void from_dtd_unq(mixed* unq) {
  error("Override from_dtd_unq to call it!");
}

/* We don't override from_dtd_unq, but we *do* provide parsing functionality
   for the really basic stuff like number and body.  Child objects may
   choose to use this.  It extracts the fields it uses, leaving the
   rest.
*/
static mixed mobile_from_dtd_unq(mixed* unq) {
  mixed *ret, *ctr;
  int    bodynum;

  ctr = unq;

  while(sizeof(ctr) > 0) {
    if(!STRINGD->stricmp(ctr[0][0], "body")) {
      bodynum = ctr[0][1];
      if(bodynum != -1) {
	body = MAPD->get_room_by_num(bodynum);
	if(!body)
	  error("Can't find body for mobile, object #" + bodynum + "!\n");
	location = body->get_location();
      } else {
	body = nil;
	location = nil;
      }
    } else if(!STRINGD->stricmp(ctr[0][0], "number")) {
      number = ctr[0][1];
    } else if(!STRINGD->stricmp(ctr[0][0], "type")) {
      /* Do nothing, already taken care of */
    } else if(!STRINGD->stricmp(ctr[0][0], "data")) {
      ret = ({ ctr[0][1] });
    } else if(!STRINGD->stricmp(ctr[0][0], "name")) {
      /* This is just a comment.  Ignore it. */
    } else {
      error("Unrecognized field in mobile structure!");
    }
    ctr = ctr[1..];
  }

  return ret;
}

string get_type(void) {
  error("Called get_type on /usr/common/lib/mobile without overriding!");
}
