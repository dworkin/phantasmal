/* $Header: /cvsroot/phantasmal/testgame/usr/game/obj/user.c,v 1.1 2003/12/05 11:38:51 angelbob Exp $ */

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
		     "yell"      : "cmd_yell",
		     "shout"     : "cmd_yell",
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
  object start_room;

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
    LOGD->write_syslog("User is already set for this mobile!",
		       LOG_ERROR);
    message("Body and mobile files are misconfigured!  Internal error!\r\n");

    body_num = -1;
    body = nil;
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
	if(sscanf(cmd, "%s %s", cmd, str) != 2) {
	  str = "";
	}
	break;
      }
    }

    if(cmd && strlen(cmd)) {
      switch (cmd) {
      case "password":
	if (password) {
	  send_system_phrase("Old password: ");
	  set_state(previous_object(), STATE_OLDPASSWD);
	} else {
	  send_system_phrase("New password: ");
	  set_state(previous_object(), STATE_NEWPASSWD1);
	}
	return MODE_NOECHO;

      case "quit":
	return MODE_DISCONNECT;
      }
    }

    if(SOULD->is_social_verb(cmd)) {
      cmd_social(this_object(), cmd, str = "" ? nil : str);
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

/* I'll move some of these in from PHANTASMAL_USER shortly */
