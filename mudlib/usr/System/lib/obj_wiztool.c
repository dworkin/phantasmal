#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/grammar.h>
#include <phantasmal/search_locations.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <status.h>
#include <limits.h>

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

static void upgraded(varargs int clone) {

}

static void destructed(varargs int clone) {

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
    
  }

  return nil;
}


/* Set object description.  This command is @set_brief,
   @set_look and @set_examine. */
static void cmd_set_obj_desc(object user, string cmd, string str) {
  object obj;
  string desc, objname;
  string look_type;
  int    must_set;

  if(!sscanf(cmd, "@set_%s", look_type))
    error("Unrecognized command to set desc: " + cmd);

  desc = nil;
  if(!str || str == "") {
    obj = user->get_location();
    user->message("(This location)\r\n");
  } else if(sscanf(str, "%s %s", objname, desc) == 2
	    || sscanf(str, "%s", objname)) {
    obj = resolve_object_name(user, objname);
  }

  if(!obj) {
    user->message("Not a valid object to be set!\r\n");
    return;
  }

  user->message("Locale is ");
  user->message(PHRASED->name_for_language(user->get_locale()) + "\r\n");

  if(!desc) {
    user->push_new_state(US_OBJ_DESC, obj, look_type, user->get_locale());
    user->push_new_state(state2, US_ENTER_DATA);

    return;
  }

  if(desc) {
    object phr;

    user->message("Setting " + look_type + " desc on object #"
		  + obj->get_number() + "\r\n");

    if(!function_object("get_" + look_type, obj))
      error("Can't find getter function for " + look_type + " in "
	    + object_name(obj));

    must_set = 0;
    phr = call_other(obj, "get_" + look_type);
    if(!phr || (look_type == "examine" && phr == obj->get_look())) {
      phr = PHRASED->new_simple_english_phrase("CHANGE ME!");
      must_set = 1;
    }
    phr->set_content_by_lang(user->get_locale(), desc);
    if(must_set) {
      call_other(obj, "set_" + look_type, phr);
    }
  }
}


private void priv_mob_stat(object user, object mob) {
  object mobbody, mobuser;
  mixed* tags;
  int    ctr;
  string tmp;

  mobbody = mob->get_body();
  mobuser = mob->get_user();

  tmp = "";

  tmp = "Body: ";
  if(mobbody) {
    tmp += "#" + mobbody->get_number() + " (";
    tmp += mobbody->get_brief()->to_string(user);
    tmp += ")\r\n";
  } else {
    tmp += "(none)\r\n";
  }

  tmp += "User: ";
  if(mobuser) {
    tmp += mobuser->get_name() + "\r\n";
  } else {
    tmp += "(NPC, not player)\r\n";
  }

  tags = TAGD->mobile_all_tags(mob);
  if(!sizeof(tags)) {
    tmp += "\r\nNo tags set.\r\n";
  } else {
    for(ctr = 0; ctr < sizeof(tags); ctr+=2) {
      tmp += "  " + tags[ctr] + ": " + STRINGD->mixed_sprint(tags[ctr + 1])
	+ "\r\n";
    }
  }

  user->message_scroll(tmp);
}


static void cmd_stat(object user, string cmd, string str) {
  int     objnum, ctr;
  object  obj, room, exit, location;
  string* words;
  string  tmp;
  mixed*  objs, *tags;
  object *details, *archetypes;

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + " #<obj num>\r\n");
    user->message("       " + cmd + " <object description>\r\n");
    return;
  }

  if(sscanf(str, "#%d", objnum) != 1) {
    str = STRINGD->trim_whitespace(str);

    if(!STRINGD->stricmp(str, "here")) {
      if(!user->get_location()) {
	user->message("You aren't anywhere!\r\n");
	return;
      }
      objnum = user->get_location()->get_number();
    } else {

      objs = user->find_first_objects(str, LOC_INVENTORY, LOC_CURRENT_ROOM,
				      LOC_BODY, LOC_CURRENT_EXITS);
      if(!objs) {
	user->message("You don't find any object matching '"
		      + str + "' here.\r\n");
	return;
      }

      if(sizeof(objs) > 1) {
	user->message("More than one object matches.  You choose one.\r\n");
      }

      objnum = objs[0]->get_number();
    }
  }

  room = MAPD->get_room_by_num(objnum);
  exit = EXITD->get_exit_by_num(objnum);

  obj = room ? room : exit;

  if(!obj) {
    object mob;

    mob = MOBILED->get_mobile_by_num(objnum);

    if(!mob) {
      user->message("No object #" + objnum
		    + " found registered with MAPD, EXITD or MOBILED.\r\n");
      return;
    }
    priv_mob_stat(user, mob);
    return;
  }

  tmp  = "Number: " + obj->get_number() + "\r\n";
  if(obj->get_detail_of()) {
    tmp += "Detail of: ";
  } else {
    tmp += "Location: ";
  }
  location = obj->get_location();
  if(location) {
    if(typeof(location->get_number()) == T_INT
       && location->get_number() != -1) {
      tmp += "#" + location->get_number();

      if(location->get_brief()) {
	tmp += " (";
	tmp += location->get_brief()->to_string(user);
	tmp += ")\r\n";
      } else {
	tmp += "\r\n";
      }
    } else {
      tmp += " (unregistered)\r\n";
    }
  } else {
    tmp += " (none)\r\n";
  }

  tmp += "Descriptions ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ")\r\n";

  tmp += "Brief: ";
  if(obj->get_brief()) {
    tmp += "'" + obj->get_brief()->to_string(user) + "'\r\n";
  } else {
    tmp += "(none)\r\n";
  }

  tmp += "Look:\r\n  ";
  if(obj->get_look()) {
    tmp += "'" + obj->get_look()->to_string(user) + "'\r\n";
  } else {
    tmp += "(none)\r\n";
  }

  tmp += "Examine:\r\n  ";
  if(obj->get_examine()) {
    if(obj->get_examine() == obj->get_look()) {
      tmp += "(defaults to Look desc)\r\n";
    } else {
      tmp += "'" + obj->get_examine()->to_string(user) + "'\r\n";
    }
  } else {
    tmp += "(none)\r\n";
  }

  /* Show user the nouns and adjectives */
  tmp += "Nouns ("
    + PHRASED->locale_name_for_language(user->get_locale())
    + "): ";
  words = obj->get_nouns(user->get_locale());
  tmp += implode(words, ", ");
  tmp += "\r\n";

  tmp += "Adjectives ("
    + PHRASED->locale_name_for_language(user->get_locale())
    + "): ";
  words = obj->get_adjectives(user->get_locale());
  tmp += implode(words, ", ");
  tmp += "\r\n\r\n";

  if(function_object("get_weight", obj)) {
    tmp += "Its weight is " + obj->get_weight() + " kilograms.\r\n";
    tmp += "Its volume is " + obj->get_volume() + " liters.\r\n";
    tmp += "Its length is " + obj->get_length() + " centimeters.\r\n";
  }

  if(function_object("is_container", obj)) {
    if(obj->is_container()) {
      if(obj->is_open()) {
	tmp += "The object is an open container.\r\n";
      } else {
	tmp += "The object is a closed container.\r\n";
      }
      if(obj->is_openable()) {
	tmp += "The object may be freely opened and closed.\r\n";
      } else {
	tmp += "The object may not be freely opened and closed.\r\n";
      }
      if(function_object("get_weight_capacity", obj)) {
	tmp += "It contains " + obj->get_current_weight()
	  + " of a max of " + obj->get_weight_capacity()
	  + " kilograms.\r\n";
	tmp += "It contains " + obj->get_current_volume()
	  + " of a max of " + obj->get_volume_capacity()
	  + " liters.\r\n";
	tmp += "Its maximum height/length is " + obj->get_length_capacity()
	  + " centimeters.\r\n";
      }
    } else {
      tmp += "The object is not a container.\r\n";
    }
  }

  tmp += "\r\n";

  if(function_object("num_objects_in_container", obj)) {
    tmp += "Contains objects [" + obj->num_objects_in_container()
		  + "]: ";
    objs = obj->objects_in_container();
    for(ctr = 0; ctr < sizeof(objs); ctr++) {
      if(typeof(objs[ctr]->get_number()) == T_INT
	 && objs[ctr]->get_number() != -1) {
	tmp += "#" + objs[ctr]->get_number() + " ";
      } else {
	tmp += "<unreg> ";
      }
    }
    tmp += "\r\nContains " + sizeof(obj->mobiles_in_container())
		  + " mobiles.\r\n\r\n";
  }

  details = obj->get_immediate_details();
  if(details && sizeof(details)) {
    object detail;

    tmp += "Has immediate details [" + sizeof(details) + "]: ";
    for(ctr = 0; ctr < sizeof(details); ctr++) {
      detail = details[ctr];
      if(detail) {
	tmp += "#" + detail->get_number() + " ";
      } else {
	tmp += "<unreg> ";
      }
    }
    tmp += "\r\n";
  }
  details = obj->get_details();
  archetypes = obj->get_archetypes();
  if(sizeof(archetypes) && sizeof(details)) {
    object detail;

    tmp += "Has complete details [" + sizeof(details) + "]: ";
    for(ctr = 0; ctr < sizeof(details); ctr++) {
      detail = details[ctr];
      if(detail) {
	tmp += "#" + detail->get_number() + " ";
      } else {
	tmp += "<unreg> ";
      }
    }
    tmp += "\r\n";
  }

  tags = TAGD->object_all_tags(obj);
  if(!sizeof(tags)) {
    tmp += "\r\nNo tags set.\r\n";
  } else {
    tmp += "\r\nTag Name: Value\r\n";
    for(ctr = 0; ctr < sizeof(tags); ctr+=2) {
      tmp += "  " + tags[ctr] + ": " + STRINGD->mixed_sprint(tags[ctr + 1])
	+ "\r\n";
    }
  }
  tmp += "\r\n";

  if(obj->get_mobile()) {
    tmp += "Object is sentient.\r\n";
    if(obj->get_mobile()->get_user()) {
      tmp += "Object is "
	+ obj->get_mobile()->get_user()->query_name()
	+ "'s body.\r\n";
    }
  }

  if(sizeof(archetypes)) {
    tmp += "Instance of archetype(s): ";
    for(ctr = 0; ctr < sizeof(archetypes); ctr++) {
      tmp += "#" + archetypes[ctr]->get_number()
	+ " (" + archetypes[ctr]->get_brief()->to_string(user)
	+ ")";
    }
    tmp += ".\r\n";
  }
  if(room) {
    tmp += "Registered with MAPD as a room or portable.\r\n";
  }
  if(exit) {
    tmp += "Registered with EXITD as an exit.\r\n";
  }

  user->message_scroll(tmp);
}


static void cmd_add_nouns(object user, string cmd, string str) {
  int     ctr;
  object  obj, phr;
  string* words;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || str == "" || !sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + " #<objnum> [<noun> <noun>...]\r\n");
    return;
  }

  words = explode(str, " ");
  obj = resolve_object_name(user, words[0]);
  if(!obj) {
    user->message("Can't find object '" + words[0] + "'.\r\n");
    return;
  }

  for(ctr = 1; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  user->message("Adding nouns ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ").\r\n");
  phr = new_object(LWO_PHRASE);
  phr->set_content_by_lang(user->get_locale(), implode(words[1..],","));
  obj->add_noun(phr);
  user->message("Done.\r\n");
}


static void cmd_clear_nouns(object user, string cmd, string str) {
  object obj;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "" || sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  obj = resolve_object_name(user, str);
  if(obj) {
    obj->clear_nouns();
    user->message("Cleared.\r\n");
  } else {
    user->message("Couldn't find object '" + str + "'.\r\n");
  }
}


static void cmd_add_adjectives(object user, string cmd, string str) {
  int     ctr;
  object  obj, phr;
  string* words;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || str == "" || !sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + " #<objnum> [<adj> <adj>...]\r\n");
    return;
  }

  words = explode(str, " ");
  obj = resolve_object_name(user, words[0]);
  if(!obj) {
    user->message("Can't find object '" + words[0] + "'.\r\n");
    return;
  }

  for(ctr = 1; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  user->message("Adding adjectives ("
		+ PHRASED->locale_name_for_language(user->get_locale())
		+ ").\r\n");
  phr = new_object(LWO_PHRASE);
  phr->set_content_by_lang(user->get_locale(), implode(words[1..],","));
  obj->add_adjective(phr);
  user->message("Done.\r\n");
}


static void cmd_clear_adjectives(object user, string cmd, string str) {
  object obj;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "" || sscanf(str, "%*s %*s")) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  obj = resolve_object_name(user, str);
  if(obj) {
    obj->clear_adjectives();
    user->message("Cleared.\r\n");
  } else {
    user->message("Couldn't find object '" + str + "'.\r\n");
  }
}


static void cmd_move_obj(object user, string cmd, string str) {
  object obj1, obj2;
  int    objnum1, objnum2;
  string second;

  if(!str || sscanf(str, "%*s %*s %*s") == 3
     || ((sscanf(str, "#%d #%d", objnum1, objnum2) != 2)
	 && (sscanf(str, "#%d %s", objnum1, second) != 2))) {
    user->message("Usage: " + cmd + " #<obj> #<location>\r\n");
    user->message("    or " + cmd + " #<obj> here\r\n");
    return;
  }

  if(second && STRINGD->stricmp(second, "here")) {
    user->message("Usage: " + cmd + " #<obj> #<location>\r\n");
    user->message("    or " + cmd + " #<obj> here\r\n");
  }

  if(second) {
    if(user->get_location()) {
      objnum2 = user->get_location()->get_number();
    } else {
      user->message("You can't move an object to your current location "
		    + "unless you have one!\r\n");
      return;
    }
  }

  obj2 = MAPD->get_room_by_num(objnum2);
  if(!obj2) {
    user->message("The second argument must be a room.  Obj #"
		  + objnum2 + " is not.\r\n");
    return;
  }

  obj1 = MAPD->get_room_by_num(objnum1);
  if(!obj1) {
    obj1 = EXITD->get_exit_by_num(objnum1);
  }

  if(!obj1) {
    user->message("Obj #" + objnum1 + " doesn't appear to be a registered "
		  + "room or exit.\r\n");
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
  user->message(" (#" + obj2->get_number() + ").\r\n");
}


static void cmd_set_obj_parent(object user, string cmd, string str) {
  object  obj, parent;
  string  parentstr, *par_entries;
  object *parents;
  int     objnum;
  mapping par_kw;

  if(!str
     || (sscanf(str, "#%d %s", objnum, parentstr) != 2)) {
    user->message("Usage: " + cmd + " #<obj> #<parent> [#<parent2>...]\r\n");
    user->message("       " + cmd + " #<obj> none\r\n");
    return;
  }

  parentstr = STRINGD->trim_whitespace(parentstr);

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("The object must be a room or portable.  Obj #"
		  + objnum + " is not.\r\n");
    return;
  }

  parentstr = STRINGD->trim_whitespace(STRINGD->to_lower(parentstr));

  if(!STRINGD->stricmp(parentstr, "none")) {
    obj->set_archetypes( ({ }) );
    user->message("Object is now unparented.\r\n");
    return;
  }

  par_entries = explode(parentstr, " ");
  parents = ({ });

  par_kw = ([ "none" : 1,
	    "add" : 1,
	    "remove" : 1 ]);

  if(!par_kw[par_entries[0]]) {
    user->message("The operation keyword must be one of 'none', 'add' or "
		  + "remove.\r\n  Your keyword, '" + par_entries[0]
		  + "', was not.\r\n  See help entry.\r\n");
    return;
  }

  for(objnum = 1; objnum < sizeof(par_entries); objnum++) {
    int parentnum;

    par_entries[objnum] = STRINGD->trim_whitespace(par_entries[objnum]);
    if(!par_entries[objnum] || par_entries[objnum] == "")
      continue;

    if(!sscanf(par_entries[objnum], "#%d", parentnum)) {
      user->message("Post-keyword arguments must be parent object numbers ("
		    + "#<num>).\r\n"
		    + "  Your argument, '" + par_entries[objnum]
		    + "', was not.\r\n");
      return;
    }

    parent = MAPD->get_room_by_num(parentnum);

    if(!parent) {
      user->message("The parent must be a room or portable.  Obj #"
		    + parentnum
		    + " is not.\r\n");
      return;
    }

    parents += ({ parent });
  }

  switch(par_entries[0]) {
  case "none":
    /* Should never get this far */
    user->message("Don't include any object numbers with the keyword 'none'."
		  + "\r\n");
    return;

  case "add":
    parents = obj->get_archetypes() + parents;
    /* Fall through to next case */

  case "set":
    obj->set_archetypes(parents);
    break;
  default:
    user->message("Internal error based on keyword name.  "
		  + "Throwing an exception.\r\n");
    error("Internal error!  Should never get here!");
  }

  user->message("Done setting parents.\r\n");
}


/* This function is called for set_obj_weight, set_obj_volume,
   and set_obj_height and their synonyms, as well as
   set_obj_weight_capacity, set_obj_height_capacity,
   set_obj_volume_capacity and so on. */
static void cmd_set_obj_value(object user, string cmd, string str) {
  object  obj;
  int     objnum, is_cap;
  float   newvalue;
  mapping val_names;
  string  cmd_value_name, cmd_norm_name;

  if(!str || sscanf(str, "%*s %*s %*s") == 3
     || sscanf(str, "#%d %f", objnum, newvalue) != 2) {
    user->message("Usage: " + cmd + " #<obj> <value>\r\n");
    return;
  }

  val_names = ([
		"weight" : "weight",
		"volume" : "volume",
		"vol"    : "volume",
		"length" : "length",
		"len"    : "length",
		"height" : "length",
		]);

  /* For set_obj_weight_capacity and company */
  if(sscanf(cmd, "%*s_%*s_%s_%*s", cmd_value_name) == 4) {
    /* Normalize the name. */
    cmd_norm_name = val_names[cmd_value_name];
    is_cap = 1;
  } else if (sscanf(cmd, "%*s_%*s_%s", cmd_value_name) == 3) {
    /* Not a set_blah_capacity function */
    cmd_norm_name = val_names[cmd_value_name];
    is_cap = 0;
  } else {
    user->message("Internal parsing error on command name '"
		  + cmd + "'.  Sorry!\r\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    user->message("The object must be a room or portable.  Obj #"
		  + objnum + " is not.\r\n");
    return;
  }

  if(newvalue < 0.0) {
    user->message("Length, weight and height values must be positive or zero."
		  + "\r\n.  " + newvalue + " is not.\r\n");
    return;
  }

  call_other(obj, "set_" + cmd_norm_name + (is_cap ? "_capacity" : ""),
	     newvalue);

  user->message("Done.\r\n");
}


static void cmd_set_obj_flag(object user, string cmd, string str) {
  object  obj, link_exit;
  int     objnum, flagval;
  string  flagname, flagstring;
  mapping valmap;

  if(str) str = STRINGD->trim_whitespace(str);
  if(str && !STRINGD->stricmp(str, "flagnames")) {
    user->message("Flag names:  cont container open openable locked lockable\r\n");
    return;
  }

  if(!str || sscanf(str, "%*s %*s %*s %*s") == 4
     || (sscanf(str, "#%d %s %s", objnum, flagname, flagstring) != 3
	 && sscanf(str, "#%d %s", objnum, flagname) != 2)) {
    user->message("Usage: " + cmd + " #<obj> flagname [flagvalue]\r\n");
    user->message("       " + cmd + " flagnames\r\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  if(!obj) {
    obj = EXITD->get_exit_by_num(objnum);
    if (!obj) {
      user->message("Can't find object #" + objnum + "!\r\n");
      return;
    }
  }

  valmap = ([
    "true": 1,
      "false": 0,
      "1": 1,
      "0": 0,
      "yes": 1,
      "no": 0 ]);

  if(flagstring) {
    if( ({ flagstring }) & map_indices(valmap) ) {
      flagval = valmap[flagstring];
    } else {
      user->message("I can't tell if value '" + flagstring
		    + "' is true or false!\r\n");
      return;
    }
  } else {
    flagval = 1;
  }

  if(!STRINGD->stricmp(flagname, "cont")
     || !STRINGD->stricmp(flagname, "container")) {
    obj->set_container(flagval);
  } else if(!STRINGD->stricmp(flagname, "open")) {
    obj->set_open(flagval);
  } else if(!STRINGD->stricmp(flagname, "openable")) {
    obj->set_openable(flagval);
  } else if(!STRINGD->stricmp(flagname, "locked")) {
    obj->set_locked(flagval);
  } else if(!STRINGD->stricmp(flagname, "lockable")) {
    obj->set_lockable(flagval);
  }

  user->message("Done.\r\n");
}


static void cmd_make_obj(object user, string cmd, string str) {
  object state;
  string typename;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  if(cmd == "@make_room") {
    typename = "room";
  } else if (cmd == "@make_port" || cmd == "@make_portable") {
    typename = "portable";
  } else if (cmd == "@make_det" || cmd == "@make_detail") {
    typename = "detail";
  } else {
    user->message("I don't recognize the kind of object "
		  + "you're trying to make.\r\n");
    return;
  }

  state = clone_object(US_MAKE_ROOM);
  if(typename)
    state->specify_type(typename);

  user->push_state(state);
}


static void cmd_set_obj_detail(object user, string cmd, string str) {
  int    objnum, detailnum;
  object obj, detail;

  if(!str
     || sscanf(str, "%*s %*s %*s") == 3
     || sscanf(str, "#%d #%d", objnum, detailnum) != 2) {
    user->message("Usage: " + cmd + " #<base_obj> #<detail_obj>\r\n");
    return;
  }

  obj = MAPD->get_room_by_num(objnum);
  detail = MAPD->get_room_by_num(detailnum);

  if(objnum != -1 && !obj) {
    user->message("Base object (#" + objnum
		  + ") doesn't appear to be a room or portable.\r\n");
  }
  if(!detail) {
    user->message("Detail object (#" + detailnum
		  + ") doesn't appear to be a room or portable.\r\n");
  }

  if(!obj || !detail) return;

  if(obj && detail->get_detail_of()) {
    user->message("Object #" + detailnum + " is already a detail of object #"
		  + detail->get_detail_of()->get_number() + "!\r\n");
    return;
  } else if(!obj && !detail->get_detail_of()) {
    user->message("Object #" + detailnum
		  + " isn't a detail of anything!\r\n");
    return;
  }

  if(obj) {
    user->message("Setting object #" + detailnum + " to be a detail of obj #"
		  + objnum + ".\r\n");
    if(detail->get_location())
      detail->get_location()->remove_from_container(detail);
    obj->add_detail(detail);
  } else {
    obj = detail->get_detail_of();

    user->message("Removing detail #" + detailnum + " from object #"
		  + obj->get_number() + ".\r\n");
    obj->remove_detail(detail);
  }

  user->message("Done.\r\n");
}
