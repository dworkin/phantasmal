#include <config.h>

#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <type.h>
#include <status.h>
#include <limits.h>
#include <log.h>

inherit access API_ACCESS;



/*
 * NAME:	create()
 * DESCRIPTION:	initialize variables
 */
static void create(varargs int clone)
{
  if(clone) {

  } else {
    if(!find_object(LWO_PHRASE))
      compile_object(LWO_PHRASE);
  }
}

void destructed(varargs int clone) {

}


/********** Object Functions ***************************************/

static object resolve_object_name(object user, string name)
{
  object obj;
  int    obnum;

  if(sscanf(name, "#%d", obnum)) {
    obj = MAPD->get_room_by_num(obnum);
    if(obj)
      return obj;
    obj = EXITD->get_exit_by_num(obnum);
    if(obj)
      return obj;
    obj = PORTABLED->get_portable_by_num(obnum);
    if(obj)
      return obj;
  }

  return nil;
}


/* Set object description.  This command is @set_brief, @set_glance,
   @set_look and @set_examine. */
static void cmd_set_obj_desc(object user, string cmd, string str) {
  object obj;
  string desc, objname;
  string look_type;

  if(!sscanf(cmd, "@set_%s", look_type))
    error("Unrecognized command to set desc: " + cmd);

  desc = nil;
  if(!str || str == "") {
    obj = user->get_location();
    user->message("(This location)\n");
  } else if(sscanf(str, "%s %s", objname, desc) == 2
	    || sscanf(str, "%s", objname)) {
    obj = resolve_object_name(user, objname);
  }

  if(!obj) {
    user->message("Not a valid object to be set!\n");
    return;
  }

  user->message("Locale is ");
  user->message(PHRASED->name_for_language(user->get_locale()) + "\n");

  if(!desc) {
    object state1, state2;

    state1 = clone_object(US_OBJ_DESC);
    state2 = clone_object(US_ENTER_DATA);

    state1->set_up_func(obj, look_type, user->get_locale());

    user->push_state(state1);
    user->push_state(state2);

    return;
  }

  if(desc) {
    object phr;

    user->message("Setting " + look_type + " desc on object #"
		  + obj->get_number() + "\n");

    if(!function_object("get_" + look_type, obj))
      error("Can't find getter function for " + look_type + " in "
	    + object_name(obj));

    phr = call_other(obj, "get_" + look_type);
    if(!phr || (look_type == "examine" && phr == obj->get_look())) {
      phr = PHRASED->new_simple_english_phrase("CHANGE ME!");
    }
    phr->set_content_by_lang(user->get_locale(), desc);
  }
}


static void cmd_stat(object user, string cmd, string str) {
  int     objnum, ctr;
  object  obj, room, port, exit, location;
  string* words;
  mixed*  objs;

  if(!str || STRINGD->is_whitespace(str) || sscanf(str, "%*s %*s") == 2
     || !sscanf(str, "#%d", objnum)) {
    user->message("Usage: " + cmd + " #<obj num>\n");
    return;
  }

  room = MAPD->get_room_by_num(objnum);
  port = PORTABLED->get_portable_by_num(objnum);
  exit = EXITD->get_exit_by_num(objnum);

  obj = room ? room : port ? port : exit;

  if(!obj) {
    user->message("No object #" + objnum
		  + " found registered with MAPD, EXITD or PORTABLED.\n");
    return;
  }

  user->message("Location: ");
  location = obj->get_location();
  if(location) {
    if(typeof(location->get_number()) == T_INT
       && location->get_number() != -1) {
      user->message("#" + location->get_number());

      if(location->get_glance()) {
	user->message(" (");
	user->send_phrase(location->get_glance());
	user->message(")\n");
      } else {
	user->message("\n");
      }
    } else {
      user->message(" (unregistered)\n");
    }
  } else {
    user->message(" (none)\n");
  }

  user->message("Descriptions ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ")\n");

  user->message("Brief:\n  ");
  if(obj->get_brief()) {
    user->message("'" + obj->get_brief()->to_string(user) + "'\n");
  } else {
    user->message("(none)\n");
  }

  user->message("Glance:\n  ");
  if(obj->get_glance()) {
    user->message("'" + obj->get_glance()->to_string(user) + "'\n");
  } else {
    user->message("(none)\n");
  }

  user->message("Look:\n  ");
  if(obj->get_look()) {
    user->message("'" + obj->get_look()->to_string(user) + "'\n");
  } else {
    user->message("(none)\n");
  }

  user->message("Examine:\n  ");
  if(obj->get_examine()) {
    if(obj->get_examine() == obj->get_look()) {
      user->message("(defaults to Look desc)\n");
    } else {
      user->message("'" + obj->get_examine()->to_string(user) + "'\n");
    }
  } else {
    user->message("(none)\n");
  }

  user->message("Nouns ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ "): ");
  words = obj->get_nouns(user->get_locale());
  user->message(implode(words, ", "));
  user->message("\n");

  user->message("Adjectives ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ "): ");
  words = obj->get_adjectives(user->get_locale());
  user->message(implode(words, ", "));
  user->message("\n");

  user->message("\n");

  if(port) {
    user->message("Portable flags:\n");
    if(port->is_container()) {
      user->message("  container:  yes\n");
    }
    if(port->is_open()) {
      user->message("  open:       yes\n");
    }
    if(port->is_no_desc()) {
      user->message("  descless:   yes\n");
    }
    user->message("\n");
  }
  if(function_object("num_objects_in_container", obj)) {
    user->message("Contains objects [" + obj->num_objects_in_container()
		  + "]: ");
    objs = obj->objects_in_container();
    for(ctr = 0; ctr < sizeof(objs); ctr++) {
      if(typeof(objs[ctr]->get_number()) == T_INT
	 && objs[ctr]->get_number() != -1) {
	user->message("#" + objs[ctr]->get_number() + " ");
      } else {
	user->message("<unreg> ");
      }
    }
    user->message("\nContains " + sizeof(obj->mobiles_in_container())
		  + " mobiles.\n\n");
  }

  if(obj->get_mobile()) {
    user->message("Object is sentient.\n");
    if(obj->get_mobile()->get_user()) {
      user->message("Object is "
		    + obj->get_mobile()->get_user()->query_name()
		    + "'s body.\n");
    }
  }

  if(room) {
    user->message("Registered with MAPD as a room.\n");
  }
  if(port) {
    user->message("Registered with PORTABLED as a portable.\n");
  }
  if(exit) {
    user->message("Registered with EXITD as an exit.\n");
  }
}


static void cmd_add_nouns(object user, string cmd, string str) {
  int     ctr;
  object  obj, phr;
  string* words;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || str == "" || !sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + " <noun> [<noun> <noun>...]\n");
    return;
  }

  words = explode(str, " ");
  obj = resolve_object_name(user, words[0]);
  if(!obj) {
    user->message("Can't find object '" + words[0] + "'.\n");
    return;
  }

  for(ctr = 1; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  user->message("Adding nouns ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ").\n");
  phr = new_object(LWO_PHRASE);
  phr->set_content_by_lang(user->get_locale(), implode(words[1..],","));
  obj->add_noun(phr);
  user->message("Done.\n");
}


static void cmd_clear_nouns(object user, string cmd, string str) {
  object obj;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "" || sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + "\n");
    return;
  }

  obj = resolve_object_name(user, str);
  if(obj) {
    obj->clear_nouns();
    user->message("Cleared.\n");
  } else {
    user->message("Couldn't find object '" + str + "'.\n");
  }
}


static void cmd_add_adjectives(object user, string cmd, string str) {
  int     ctr;
  object  obj, phr;
  string* words;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || str == "" || !sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + " #<objnum> [<adj> <adj>...]\n");
    return;
  }

  words = explode(str, " ");
  obj = resolve_object_name(user, words[0]);
  if(!obj) {
    user->message("Can't find object '" + words[0] + "'.\n");
    return;
  }

  for(ctr = 1; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  user->message("Adding adjectives ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ").\n");
  phr = new_object(LWO_PHRASE);
  phr->set_content_by_lang(user->get_locale(), implode(words[1..],","));
  obj->add_adjective(phr);
  user->message("Done.\n");
}


static void cmd_clear_adjectives(object user, string cmd, string str) {
  object obj;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "" || sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + "\n");
    return;
  }

  obj = resolve_object_name(user, str);
  if(obj) {
    obj->clear_adjectives();
    user->message("Cleared.\n");
  } else {
    user->message("Couldn't find object '" + str + "'.\n");
  }
}


static void cmd_move_obj(object user, string cmd, string str) {
  object obj1, obj2;
  int    objnum1, objnum2;

  if(!str || sscanf(str, "%*s %*s %*s") == 3
     || sscanf(str, "#%d #%d", objnum1, objnum2) != 2) {
    user->message("Usage: " + cmd + " #<obj> #<location>\n");
    return;
  }

  obj2 = MAPD->get_room_by_num(objnum2);
  if(!obj2) {
    obj2 = PORTABLED->get_portable_by_num(objnum2);
  }
  if(!obj2) {
    user->message("The second argument must be a room or portable.  Obj #"
		  + objnum2 + " is not.\n");
    return;
  }

  obj1 = MAPD->get_room_by_num(objnum1);
  if(!obj1) {
    obj1 = PORTABLED->get_portable_by_num(objnum1);
  }
  if(!obj1) {
    obj1 = EXITD->get_exit_by_num(objnum1);
  }

  if(!obj1) {
    user->message("Obj #" + objnum1 + " doesn't appear to be a registered "
		  + "room, portable or exit.\n");
    return;
  }

  if(obj1->get_location()) {
    obj1->get_location()->remove_from_container(obj1);
  }
  obj2->add_to_container(obj1);

  user->message("You teleport ");
  user->send_phrase(obj1->get_brief());
  user->message(" (#" + obj1->get_number() + ") into ");
  user->send_phrase(obj2->get_brief());
  user->message(" (#" + obj2->get_number() + ").\n");
}
