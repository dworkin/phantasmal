/* $Header: /cvsroot/phantasmal/testgame/usr/game/obj/user.c,v 1.2 2003/12/08 21:39:28 angelbob Exp $ */

#include <kernel/kernel.h>
#include <kernel/user.h>
#include <kernel/rsrc.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/channel.h>
#include <phantasmal/map.h>
#include <phantasmal/search_locations.h>

#include <config.h>
#include <type.h>

inherit PHANTASMAL_USER;

/* Duplicated in PHANTASMAL_USER */
#define STATE_NORMAL            0
#define STATE_LOGIN             1
#define STATE_OLDPASSWD         2
#define STATE_NEWPASSWD1        3
#define STATE_NEWPASSWD2        4

static mapping commands_map;

/* Prototypes */
       void upgraded(varargs int clone);
static void cmd_social(object user, string cmd, string str);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)


/*
 * NAME:	create()
 * DESCRIPTION:	initialize user object
 */
static void create(int clone)
{
  ::create(clone);
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    ::upgraded(clone);

    commands_map = ([
		     "say"       : "cmd_say",
		     "emote"     : "cmd_emote",
		     "ooc"       : "cmd_ooc",
		     "impbug"    : "cmd_impbug",

		     "n"         : "cmd_movement",
		     "s"         : "cmd_movement",
		     "e"         : "cmd_movement",
		     "w"         : "cmd_movement",
		     "nw"        : "cmd_movement",
		     "sw"        : "cmd_movement",
		     "ne"        : "cmd_movement",
		     "se"        : "cmd_movement",
		     "u"         : "cmd_movement",
		     "d"         : "cmd_movement",

		     "north"     : "cmd_movement",
		     "south"     : "cmd_movement",
		     "east"      : "cmd_movement",
		     "west"      : "cmd_movement",
		     "northwest" : "cmd_movement",
		     "southwest" : "cmd_movement",
		     "northeast" : "cmd_movement",
		     "southeast" : "cmd_movement",
		     "up"        : "cmd_movement",
		     "down"      : "cmd_movement",
		     "in"        : "cmd_movement",
		     "out"       : "cmd_movement",

		     "help"      : "cmd_help",
		     "locale"    : "cmd_locale",
		     "locales"   : "cmd_locales",
		     "users"     : "cmd_users",
		     "who"       : "cmd_users",
		     "whoami"    : "cmd_whoami",
		     "bug"       : "cmd_bug",
		     "typo"      : "cmd_typo",
		     "idea"      : "cmd_idea",
		     "tell"      : "cmd_tell",
		     "lines"     : "cmd_set_lines",
		     "set_lines" : "cmd_set_lines",

		     "channel"   : "cmd_channels",
		     "channels"  : "cmd_channels",

		     "g"         : "cmd_look",
		     "gl"        : "cmd_look",
		     "glance"    : "cmd_look",
		     "l"         : "cmd_look",
		     "look"      : "cmd_look",
		     "ex"        : "cmd_look",
		     "exa"       : "cmd_look",
		     "examine"   : "cmd_look",

		     "get"       : "cmd_get",
		     "take"      : "cmd_get",
		     "drop"      : "cmd_drop",
		     "i"         : "cmd_inventory",
		     "inv"       : "cmd_inventory",
		     "inventory" : "cmd_inventory",
		     "put"       : "cmd_put",
		     "place"     : "cmd_put",
		     "remove"    : "cmd_remove",
		     "open"      : "cmd_open",
		     "close"     : "cmd_close",

		     "parse"     : "cmd_parse"
    ]);

  }
}

/****************/


/*
 * Returns true if the name isn't allowed
 */
int name_is_forbidden(string name) {
  string filename;

  if(previous_program() != PHANTASMAL_USER)
    return 1;

  /* No trailing spaces or slashes in names allowed */
  if (!name || strlen(name) == 0 || sscanf(name, "%*s ") != 0 ||
      sscanf(name, "%*s/") != 0) {
    return 1;
  }

  filename = username_to_filename(name);
  filename = STRINGD->to_lower(filename);

  /* These are all bad ideas for security reasons */
  if(filename == "" || filename == nil)
    return 1;
  if(filename == "game")
    return 1;
  if(sscanf(filename, "common"))
    return 1;
  if(sscanf(filename, "system"))
    return 1;
}


/*
 * NAME:	player_login()
 * DESCRIPTION:	Create the player body, set the account info and so on...
 */
void player_login(void)
{
  int    start_room_num, start_zone;
  object start_room, other_user;

  if(previous_program() != PHANTASMAL_USER)
    return;

  body = nil;

  /* Set up location, body, etc */
  start_room_num = CONFIGD->get_start_room();
  start_room = MAPD->get_room_by_num(start_room_num);

  /* If start room can't be found, set the start room to the void */
  if (start_room == nil) {
    LOGD->write_syslog("Can't find the start room!  Starting in the void...");
    start_room_num = 0;
    start_room = MAPD->get_room_by_num(start_room_num);
    start_zone = 0;
    if(start_room == nil) {
      /* Panic!  No void! */
      error("Internal Error: no Void!");
    }
  } else {
    start_zone = ZONED->get_zone_for_room(start_room);
    if(start_zone < 0) {
      /* What's with this start room? */
      error("Internal Error:  no zone, not even zero, for start room!");
    }
  }

  if(body_num > 0) {
    body = MAPD->get_room_by_num(body_num);
  }

  if(body && body->get_mobile()
     && body->get_mobile()->get_user()) {
    other_user = body->get_mobile()->get_user();
  }
  if(other_user && other_user->get_name() != name) {
    LOGD->write_syslog("User is already set for this mobile!",
		       LOG_ERROR);
    message("Body and mobile files are misconfigured!  Internal error!\r\n");

    other_user->message(
	       "Somebody has logged in with your name and account!\r\n");
    other_user->message("Closing your connection now...\r\n");
    destruct_object(other_user);
  }


  if(!body) {
    location = start_room;

    body = clone_object(SIMPLE_ROOM);
    if(!body)
      error("Can't clone player's body!");

    body->set_container(1);
    body->set_open(1);
    body->set_openable(0);

    /* Players weigh about 80 kilograms */
    body->set_weight(80.0);
    /* Players are about 2.5dm x 1dm x 18dm == 45dm^3 == 45 liters */
    body->set_volume(45.0);
    /* A player is about 18dm == 180 centimeters tall */
    body->set_length(180.0);

    /* Players are able to lift 50 kilograms */
    body->set_weight_capacity(50.0);
    /* Players are able to carry up to 20 liters of stuff --
       that's roughly a large hiking backpack. */
    body->set_volume_capacity(20.0);
    /* Players are able to carry an object up to half a meter long.
       Note that that's stuff they're not currently holding in their
       hands, so that's more reasonable.  Can you fit a 60cm object
       in your pocket?  Would you want to? */
    body->set_length_capacity(50.0);

    MAPD->add_room_to_zone(body, -1, start_zone);
    if(!MAPD->get_room_by_num(body->get_number())) {
      LOGD->write_syslog("Error making new body!", LOG_ERR);
    }
    body_num = body->get_number();

    /* Set descriptions and add noun for new name */
    body->set_brief(NEW_PHRASE(Name));
    body->set_glance(NEW_PHRASE(Name));
    body->set_look(NEW_PHRASE(Name + " wanders the MUD."));
    body->set_examine(nil);
    body->add_noun(NEW_PHRASE(STRINGD->to_lower(name)));

    /* Can't just clone mobile here, it causes problems later */
    mobile = MOBILED->clone_mobile_by_type("user");
    if(!mobile)
      error("Can't clone mobile of type 'user'!");
    MOBILED->add_mobile_number(mobile, -1);
    mobile->assign_body(body);
    mobile->set_user(this_object());

    mobile->teleport(location, 1);

    /* We just set a body number, so we need to save the player data
       file again... */
    save_user_to_file();
  } else {
    location = body->get_location();
    mobile = body->get_mobile();
    if(!mobile) {
      mobile = clone_object(USER_MOBILE);
      MOBILED->add_mobile_number(mobile, -1);
      mobile->assign_body(body);
    }

    mobile->set_user(this_object());
    mobile->teleport(location, 1);

    /* Move body to start room */
    if(location->get_number() == CONFIGD->get_meat_locker()) {
      mobile->teleport(start_room, 1);
    }
  }

  /* Show room to player */
  message("\r\n");
  show_room_to_player(location);
}


/*
 * NAME:	player_logout()
 * DESCRIPTION:	Deal with player body, update account info and so on...
 */
private void player_logout(void)
{
  if(previous_program() != PHANTASMAL_USER)
    return;

  /* Teleport body to meat locker */
  if(body) {
    object meat_locker;
    int    ml_num;
    object mobile;

    ml_num = CONFIGD->get_meat_locker();
    if(ml_num >= 0) {
      meat_locker = MAPD->get_room_by_num(ml_num);
      if(meat_locker) {
	if (location) {
	  mobile = body->get_mobile();
	  mobile->teleport(meat_locker, 1);
	}
      } else {
	LOGD->write_syslog("Can't find room #" + ml_num + " as meat locker!",
			   LOG_ERR);
      }
    }
  }

  CHANNELD->unsubscribe_user_from_all(this_object());
}


int process_command(string str)
{
  string cmd;
  int    ctr, size;
  int    force_command;
  mixed* command;

  cmd = str;
  if (strlen(str) != 0 && str[0] == '!') {
    cmd = cmd[1 ..];
    force_command = 1;
  }

  /* Do this unless we're in the editor and didn't start the command
     with an exclamation mark */
  if (!wiztool || !query_editor(wiztool) || force_command) {
    /* check standard commands */
    cmd = STRINGD->trim_whitespace(cmd);
    if(cmd == "") {
      str = nil;
    }

    if (strlen(cmd) != 0) {
      switch (cmd[0]) {
      case '\'':
	if (strlen(cmd) > 1) {
	  str = cmd[1..];
	} else {
	  str = "";
	}
	cmd = "say";
	break;

      case ':':
	if (strlen(cmd) > 1) {
	  str = cmd[1..];
	} else {
	  str = "";
	}
	cmd = "emote";
	break;

      default:
	/* If single word, leave cmd the same.  If multiword, put
	   first word in cmd. */
	if(sscanf(cmd, "%s %s", cmd, str) != 2) {
	  str = "";
	}
	break;
      }
    }

    if(cmd && strlen(cmd)) {
      switch (cmd) {
      case "password":
	if(str && strlen(str))
	  message("(Arguments ignored...)\r\n");

	if (password) {
	  send_system_phrase("Old password: ");
	  set_state(previous_object(), STATE_OLDPASSWD);
	} else {
	  send_system_phrase("New password: ");
	  set_state(previous_object(), STATE_NEWPASSWD1);
	}
	return MODE_NOECHO;

      case "quit":
	if(str && strlen(str))
	  message("(Arguments ignored...)\r\n");

	return MODE_DISCONNECT;
      }
    }

    if(SOULD->is_social_verb(cmd)) {
      cmd_social(this_object(), cmd, str = "" ? nil : str);
      str = nil;
    }

    if(commands_map[cmd]) {
      call_other(this_object(),                /* Call on self */
		 commands_map[cmd],            /* The function */
		 this_object(),                /* This user */
		 cmd,                          /* The command */
		 str == "" ? nil : str);       /* str or nil */
      str = nil;
    }
  }

  if (str) {
    if (wiztool) {
      wiztool->command(cmd, str);
    } else {
      send_system_phrase("No match");
      message(": " + cmd + " " + str + "\r\n");
    }
  }

  /* All is well, just print a prompt and wait for next command */
  return -1;
}


/************** User-level commands *************************/

/* Temporary command for testing the parser */
static void cmd_parse(object user, string cmd, string str) {
  mixed output;

  output = PARSED->parse_cmd(str);

  if (output == nil) {
    message("FAILED!\r\n");
  } else {
    message("PARSED!\r\n" + STRINGD->tree_sprint(output, 0) + "\r\n");
  }
}

static void cmd_set_lines(object user, string cmd, string str) {
  int new_num_lines;

  if(!str || str == ""
     || sscanf(str, "%*s %*s") == 2
     || sscanf(str, "%d", new_num_lines) != 1) {
    send_system_phrase("Usage: ");
    message(cmd + " <num lines>\r\n");
    return;
  }

  set_num_lines(new_num_lines);
  message("Set number of lines to " + new_num_lines + ".\r\n");
}

static void cmd_ooc(object user, string cmd, string str) {
  if (!str || str == "") {
    send_system_phrase("Usage: ");
    message(cmd + " <text>\r\n");
    return;
  }

  CHANNELD->chat_to_channel(CHANNEL_OOC, NEW_PHRASE(str));

  send_system_phrase("(OOC)");
  message(" ");
  send_system_phrase("You chat");
  message(": " + str + "\r\n");
}

static void cmd_say(object user, string cmd, string str) {
  if(!str || str == "") {
    send_system_phrase("Usage: ");
    message(cmd + " ");
    send_system_phrase("<text>");
    message("\r\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  mobile->say(str);
}

static void cmd_emote(object user, string cmd, string str) {
  if(!str || str == "") {
    send_system_phrase("Usage: ");
    message(cmd + " ");
    send_system_phrase("<text>");
    message("\r\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  mobile->emote(str);
}

/* TODO:  remove public inheritance of USER_API? */
static void cmd_tell(object self, string cmd, string str) {
  object user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2 ||
      !(user=::find_user(username))) {
    send_system_phrase("Usage: ");
    message(cmd + " <user> <text>\r\n");
  } else {
    user->message(Name + " tells you: " + str + "\r\n");
  }
}

static void cmd_impbug(object user, string cmd, string str) {
  message("Unimplemented.  Use ooc.\r\n");
}

static void cmd_whoami(object user, string cmd, string str) {
  message("You are '" + name + "'.\r\n");
}

static void cmd_locale(object user, string cmd, string str) {
  int loc;
  string lstr;

  if (!str || STRINGD->is_whitespace(str)) {
    /* Was only one word, the command itself */
    lstr = PHRASED->name_for_language(get_locale());
    message("Current locale: " + lstr + "\r\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);

  /* Should only be one word -- the locale itself */
  if (sscanf(str, "%*s %*s") == 2) {
    /* Oops, more than one left, exit with error */
    send_system_phrase("Usage: ");
    message("locale <dialect>\r\n");
    return;
  }    

  loc = PHRASED->language_by_name(str);
  if(loc == -1) {
    message("Unrecognized language or dialect: " + str + ".\r\n");
    return;
  }
  set_locale(loc);
  message("Setting locale to " + PHRASED->name_for_language(loc)
	  + ".\r\n");
  save_user_to_file();
}

static void cmd_locales(object user, string cmd, string str) {
  message("Valid locales:\r\n  english\r\n  espanol\r\n\r\n");
}

static void cmd_look(object user, string cmd, string str) {
  object* tmp, *objs;
  int     ctr;

  str = STRINGD->trim_whitespace(str);

  if(!location) {
    user->message("You're nowhere!\r\n");
    return;
  }

  if(!str || str == "") {
    show_room_to_player(location);
    return;
  }

  if (cmd[0] != 'e') {
    /* trim an initial "at" off the front of the command if the verb
       was "look" and not "examine". */
    sscanf(str, "at %s", str);
  }

  if(sscanf(str, "in %s", str) || sscanf(str, "inside %s", str)
     || sscanf(str, "within %s", str) || sscanf(str, "into %s", str)) {
    /* Look inside container */
    str = STRINGD->trim_whitespace(str);
    tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
    if(!tmp) {
      user->message("You don't find any '" + str + "'.\r\n");
      return;
    }
    if(sizeof(tmp) > 1) {
      user->message("You see more than one '" + str +"'.  You pick one.\r\n");
    }

    if(!tmp[0]->is_container()) {
      user->message("That's not a container.\r\n");
      return;
    }

    if(!tmp[0]->is_open()) {
      user->message("It's closed.\r\n");
      return;
    }

    objs = tmp[0]->objects_in_container();
    if(objs && sizeof(objs)) {
      for(ctr = 0; ctr < sizeof(objs); ctr++) {
        user->message("- ");
        user->send_phrase(objs[ctr]->get_brief());
        user->message("\r\n");
      }
    user->message("-----\r\n");
    } else {
      user->message("You see nothing in the ");
      user->send_phrase(tmp[0]->get_brief());
      user->message(".\r\n");
    }
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_BODY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    user->message("You don't find any '" + str + "'.\r\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    user->message("More than one of those is here.  "
		  + "You check the first one.\r\n\r\n");
  }

  if(cmd[0] == 'g') {
    user->send_phrase(tmp[0]->get_glance());
  } else if(cmd[0] == 'e' && tmp[0]->get_examine()) {
    user->send_phrase(tmp[0]->get_examine());
  } else {
    user->send_phrase(tmp[0]->get_look());
  }
  user->message("\r\n");
}

static void cmd_inventory(object user, string cmd, string str) {
  int    ctr;
  mixed* objs;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  objs = body->objects_in_container();
  if(!objs || !sizeof(objs)) {
    user->message("You're empty-handed.\r\n");
    return;
  }
  for(ctr = 0; ctr < sizeof(objs); ctr++) {
    user->message("- ");
    user->send_phrase(objs[ctr]->get_glance());
    user->message("\r\n");
  }
}

static void cmd_put(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Usage: " + cmd + " <obj1> in <obj2>\r\n");
    return;
  }

  if(sscanf(str, "%s inside %s", obj1, obj2) != 2
     && sscanf(str, "%s into %s", obj1, obj2) != 2
     && sscanf(str, "%s in %s", obj1, obj2) != 2) {
    user->message("Usage: " + cmd + " <obj1> in <obj2>\r\n");
    return;
  }

  portlist = find_first_objects(obj1, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!portlist || !sizeof(portlist)) {
    user->message("You can't find any '" + obj1 + "' here.\r\n");
    return;
  }

  contlist = find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!contlist || !sizeof(contlist)) {
    user->message("You can't find any '" + obj2 + "' here.\r\n");
    return;
  }

  if(sizeof(portlist) > 1) {
    user->message("More than one object fits '" + obj1 + "'.  "
		  + "You pick " + portlist[0]->get_glance() + ".\r\n");
  }

  if(sizeof(contlist) > 1) {
    user->message("More than one open container fits '" + obj2 + "'.  "
		  + "You pick " + portlist[0]->get_glance() + ".\r\n");
  }

  port = portlist[0];
  cont = contlist[0];

  if (!(err = mobile->place(port, cont))) {
    user->message("You put ");
    user->send_phrase(port->get_brief());
    user->message(" in ");
    user->send_phrase(cont->get_brief());
    user->message(".\r\n");
  } else {
    user->message(err + "\r\n");
  }

}

static void cmd_remove(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    user->message("Usage: " + cmd + " <obj1> from <obj2>\r\n");
    return;
  }

  if(sscanf(str, "%s from inside %s", obj1, obj2) != 2
     && sscanf(str, "%s from in %s", obj1, obj2) != 2
     && sscanf(str, "%s from %s", obj1, obj2) != 2
     && sscanf(str, "%s out of %s", obj1, obj2) != 2) {
    user->message("Usage: " + cmd + " <obj1> from <obj2>\r\n");
    return;
  }

  contlist = find_first_objects(obj2, LOC_INVENTORY, LOC_CURRENT_ROOM,
				LOC_BODY);
  if(!contlist || !sizeof(contlist)) {
    user->message("You can't find any '" + obj2 + "' here.\r\n");
    return;
  }

  if(sizeof(contlist) > 1) {
    user->message("More than one open container fits '" + obj2 + "'.\r\n");
    user->message("You pick " + contlist[0]->get_glance() + ".\r\n");
  }
  cont = contlist[0];

  portlist = cont->find_contained_objects(user, obj1);
  if(!portlist || !sizeof(portlist)) {
    user->message("You can't find any '" + obj1 + "' in ");
    user->send_phrase(cont->get_brief());
    user->message(".\r\n");
    return;
  }

  if(sizeof(portlist) > 1) {
    user->message("More than one object fits '" + obj1 + "'.\r\n");
    user->message("You pick " + portlist[0]->get_glance() + ".\r\n");
  }
  port = portlist[0];

  if (!(err = mobile->place(port, body))) {
    user->message("You " + cmd + " ");
    user->send_phrase(port->get_brief());
    user->message(" from ");
    user->send_phrase(cont->get_brief());
    user->message(" (taken).\r\n");
  } else {
    user->message(err + "\r\n");
  }
}


static void cmd_users(object user, string cmd, string str) {
  int i, sz;
  object* users;
  string name_idx;

  users = users();
  send_system_phrase("Logged on:");
  message("\r\n");
  str = "";
  for (i = 0, sz = sizeof(users); i < sz; i++) {
    name_idx = users[i]->query_name();
    if (name_idx) {
      str += "   " + name_idx + "       Idle: " + users[i]->get_idle_time()
	+ " seconds\r\n";
    }
  }
  message(str + "\r\n");
}

private string bug_header(string cmd, object user) {
  string ret;
  object location;

  location = user->get_location();

  ret = "\n" + ctime(time()) + ": " + cmd + " Report\n";
  ret += "Reported by user " + STRINGD->mixed_sprint(user->get_Name()) + "\n";
  catch {
    if(location) {
      /* Currently, include the room name the way the user sees it.
	 Could even be useful for debugging internationalization
	 problems. */
      ret += "In room #" + STRINGD->mixed_sprint(location->get_number())
	+ " (" + location->get_brief()->to_string(user) + ")\n";
    } else {
      ret += "In no location at all!\n";
    }
  } : {
    ret += "(Error trying to get location in " + cmd + " report!)\n";
  }

  return ret;
}

static void cmd_bug(object user, string cmd, string str) {
  write_file(BUG_DATA, bug_header("bug", user) + str + "\n");

  message("Reported bug: " + str + "\r\n");
}

static void cmd_idea(object user, string cmd, string str) {
  write_file(IDEA_DATA, bug_header("idea", user) + str + "\n");

  message("Reported idea: " + str + "\r\n");
}

static void cmd_typo(object user, string cmd, string str) {
  write_file(TYPO_DATA, bug_header("typo", user) + str + "\n");

  message("Reported typo: " + str + "\r\n");
}

static void cmd_movement(object user, string cmd, string str) {
  int    dir;
  string reason;

  /* Currently, we ignore modifiers (str) and just move */

  dir = EXITD->direction_by_string(cmd);
  if(dir == -1) {
    user->message("'" + cmd + "' doesn't look like a valid direction.\r\n");
    return;
  }

  if (reason = mobile->move(dir)) {
    user->message(reason + "\r\n");

    /* don't show the room to the player if they havn't gone anywhere */
    return;
  }

  show_room_to_player(location);
}


/* This one is special, and is called specially... */
static void cmd_social(object user, string cmd, string str) {
  object* targets;

  if(!SOULD->is_social_verb(cmd)) {
    message(cmd + " doesn't look like a valid social verb.\r\n");
    return;
  }

  if(str && str != "") {
    targets = location->find_contained_objects(user, str);
    if(!targets) {
      message("You don't see any objects matching '" + str
	      + "' here.\r\n");
      return;
    }

    /* For the moment, just pick the first one */
    mobile->social(cmd, targets[0]);
    return;
  }

  mobile->social(cmd, nil);
}

static void cmd_get(object user, string cmd, string str) {
  object* tmp;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Usage: " + cmd + " <description>\r\n");
    return;
  }

  if(sscanf(str, "%*s from inside %*s") == 2
     || sscanf(str, "%*s from in %*s") == 2
     || sscanf(str, "%*s from %*s") == 2
     || sscanf(str, "%*s out of %*s") == 2) {
    cmd_remove(user, cmd, str);
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY);
  if(!tmp || !sizeof(tmp)) {
    message("You don't find any '" + str + "'.\r\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    message("More than one of those is here.\r\n");
    message("You choose ");
    send_phrase(tmp[0]->get_glance());
    message(".\r\n");
  }

  if(tmp[0] == location) {
    message("You can't get that.  You're standing inside it.\r\n");
    return;
  }

  if(tmp[0]->get_detail_of()) {
    message("You can't get that.  It's part of ");
    send_phrase(tmp[0]->get_detail_of()->get_glance());
    message(".\r\n");
    return;
  }

  if(!(err = mobile->place(tmp[0], body))) {
    message("You " + cmd + " ");
    send_phrase(tmp[0]->get_glance());
    message(".\r\n");
  } else {
    message(err + "\r\n");
  }
}

static void cmd_drop(object user, string cmd, string str) {
  object* tmp;
  string err;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Usage: " + cmd + " <description>\r\n");
    return;
  }

  tmp = find_first_objects(str, LOC_INVENTORY, LOC_BODY);
  if(!tmp || !sizeof(tmp)) {
    message("You're not carrying any '" + str + "'.\r\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    message("You have more than one of those.\r\n");
    message("You drop " + tmp[0]->get_glance() + ".\r\n");
  }

  if (!(err = mobile->place(tmp[0], location))) {
    message("You drop ");
    send_phrase(tmp[0]->get_glance());
    message(".\r\n");
  } else {
    message(err + "\r\n");
  }
}

static void cmd_open(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Usage: " + cmd + " <description>\r\n");
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_INVENTORY, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    message("You don't find any '" + str + "'.\r\n");
    return;
  }

  ctr = 0;
  if(sizeof(tmp) > 1) {
    for(ctr = 0; ctr < sizeof(tmp); ctr++) {
      if(tmp[ctr]->is_openable())
	break;
    }
    if(ctr >= sizeof(tmp)) {
      message("None of those can be opened.\r\n");
      return;
    }

    message("More than one of those is here.\r\n");
    message("You choose ");
    send_phrase(tmp[ctr]->get_glance());
    message(".\r\n");
  }

  if(!tmp[ctr]->is_openable()) {
    message("You can't open that!\r\n");
    return;
  }

  if(!(err = mobile->open(tmp[ctr]))) {
    message("You open ");
    send_phrase(tmp[0]->get_glance());
    message(".\r\n");
  } else {
    message(err + "\r\n");
  }
}

static void cmd_close(object user, string cmd, string str) {
  object* tmp;
  string  err;
  int     ctr;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Usage: " + cmd + " <description>\r\n");
    return;
  }

  tmp = find_first_objects(str, LOC_CURRENT_ROOM, LOC_CURRENT_EXITS);
  if(!tmp || !sizeof(tmp)) {
    message("You don't find any '" + str + "'.\r\n");
    return;
  }

  ctr = 0;
  if(sizeof(tmp) > 1) {
    for(ctr = 0; ctr < sizeof(tmp); ctr++) {
      if(tmp[ctr]->is_openable())
	break;
    }
    if(ctr >= sizeof(tmp)) {
      message("None of those can be opened.\r\n");
      return;
    }

    message("More than one of those is here.\r\n");
    message("You choose ");
    send_phrase(tmp[ctr]->get_glance());
    message(".\r\n");
  }

  if(!tmp[ctr]->is_openable()) {
    message("You can't close that!\r\n");
    return;
  }

  if(!(err = mobile->close(tmp[ctr]))) {
    message("You close ");
    send_phrase(tmp[0]->get_glance());
    message(".\r\n");
  } else {
    message(err + "\r\n");
  }
}
