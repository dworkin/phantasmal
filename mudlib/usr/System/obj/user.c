/* $Header: /cvsroot/phantasmal/mudlib/usr/System/obj/user.c,v 1.57 2003/03/28 20:52:18 dbd22 Exp $ */

#include <kernel/kernel.h>
#include <kernel/user.h>
#include <kernel/rsrc.h>
#include <config.h>
#include <type.h>
#include <log.h>
#include <phrase.h>
#include <channel.h>
#include <map.h>
#include <body_loc.h>

inherit LIB_USER;
inherit user API_USER;
inherit rsrc API_RSRC;
inherit cmd  SYSTEM_COMMANDSETLIB;
inherit io   SYSTEM_USER_IO;

#define STATE_NORMAL            0
#define STATE_LOGIN             1
#define STATE_OLDPASSWD         2
#define STATE_NEWPASSWD1        3
#define STATE_NEWPASSWD2        4

/* Saved by save_object */
string name;	                /* user name */
string password;		/* user password */
int    locale;                  /* chosen output locale */
int    channel_subs;            /* user channel subscriptions */
int    log_chan_level;          /* Level of output on CHANNEL_LOG */
int    err_chan_level;          /* Level of output on CHANNEL_ERR */
int    body_num;                /* number of body object */


/* Random unsaved */
static string Name;		/* capitalized user name */
static string newpasswd;	/* new password */
static object wiztool;		/* command handler */
static int nconn;		/* # of connections */
static int timestamp;           /* Last network input */
static string hostname;         /* Hostname they're logging in from */

/* Cached vars */
static object body;             /* Body object */
static object mobile;           /* Mobile for body object */
static object location;         /* Location of body */

/* Prototypes */
        void   upgraded(void);
static  int    process_message(string str);
static  void   print_prompt(void);
private int    name_is_forbidden(string name);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)


/*
 * NAME:	create()
 * DESCRIPTION:	initialize user object
 */
static void create(int clone)
{
  if (clone) {
    user::create();
    rsrc::create();
    cmd::create();
    io::create();

    /* Default to enUS locale */
    locale = PHRASED->language_by_name("english");
  } else {
    upgraded();
  }
}

void upgraded(void) {
  if(!find_object(SYSTEM_WIZTOOL)) { compile_object(SYSTEM_WIZTOOL); }
  if(!find_object(USER_MOBILE)) { compile_object(USER_MOBILE); }
  if(!find_object(SIMPLE_ROOM)) { compile_object(SIMPLE_ROOM); }

  cmd::upgraded();
  io::upgraded();
}


/****** Accessor functions *******/

int get_locale(void) {
  return locale;
}

void set_locale(int new_loc) {
  locale = new_loc;
}

string get_name(void) {
  return name;
}

string get_Name(void) {
  return Name;
}

object get_location(void) {
  return location;
}

object get_body(void) {
  return body;
}

int get_idle_time(void) {
  return time() - timestamp;
}

int is_admin(void)  {

  /* Can't just check wiztool's existence because we need to be able to
     restore subscribed admin-only channels using the
     restore_player_from_file functionality. */

  /* Check if an immort */
  if (sizeof(rsrc::query_owners() & ({ name }))) {
    return 1;
  }

  return 0;
}

/****************/

static string username_to_filename(string str) {
  int iter;
  int len;
  string ret;

  ret = "";
  len = strlen(str);
  for(iter = 0; iter < len; iter++) {
    if(str[iter] >= 'a' && str[iter] <= 'z')
      ret += str[iter..iter];
    else if(str[iter] >= 'A' && str[iter] <= 'Z') {
      str[iter] += 'a' - 'A';
      ret += str[iter..iter];
    }
  }
  return ret;
}

static void save_user_to_file(void) {
  mixed*  chanlist;
  int     subcode, ctr;

  chanlist = CHANNELD->channel_list(this_object());
  subcode = 0;
  for(ctr = 0; ctr < sizeof(chanlist); ctr++) {
    if(CHANNELD->is_subscribed(this_object(), chanlist[ctr][1])) {
      if(chanlist[ctr][1] == CHANNEL_LOG) {
	log_chan_level = CHANNELD->sub_data_level(this_object(),
						  chanlist[ctr][1]);
      } else if (chanlist[ctr][1] == CHANNEL_ERR) {
	err_chan_level = CHANNELD->sub_data_level(this_object(),
						  chanlist[ctr][1]);
      }

      subcode += 1 << chanlist[ctr][1];
    }
  }

  channel_subs = subcode;
  save_object(SYSTEM_USER_DIR + "/" + username_to_filename(name) + ".pwd");
}

static void restore_user_from_file(string str) {
  object* chanlist;
  int     chan_code, channel, subcode;

  restore_object(SYSTEM_USER_DIR + "/" + username_to_filename(str) + ".pwd");

  CHANNELD->unsubscribe_user_from_all(this_object());

  chan_code = 1;
  channel = 0;
  subcode = channel_subs;
  while(subcode && subcode >= chan_code) {
    /* If this user should be subbed to this channel */
    if(subcode & chan_code) {
      int ret;

      subcode -= chan_code;

      switch(channel) {
      case CHANNEL_LOG:
	ret = CHANNELD->subscribe_user(this_object(), channel, log_chan_level);
	break;
      case CHANNEL_ERR:
	ret = CHANNELD->subscribe_user(this_object(), channel, err_chan_level);
	break;
      default:
	ret = CHANNELD->subscribe_user(this_object(), channel);
      }

      if(ret < 0) {
	LOGD->write_syslog("Couldn't sub user " + Name + " to channel #"
			   + channel + "!", LOG_ERROR);
      }
    }

    /* Next channel... */
    chan_code += chan_code;
    channel++;
  }
  if(subcode != 0) {
    LOGD->write_syslog("Subcode " + subcode + " means user didn't sub to all "
		       + "appropriate channels!", LOG_WARNING);
  }
}


/* This does a lowest-level, unfiltered send to the connection object
   itself.  Normally message will be filtered through the user_state
   object(s) active, if any */
int send_string(string str) {
  if(SYSTEM() || previous_program() == USER_STATE) {
    return ::message(str);
  } else {
    error("Can't send_string from object " + previous_program());
  }
}

void user_state_data(mixed data) {
  if(data == nil) {
    /* Passing nil means "done now, print a prompt". */
    print_prompt();
    return;
  }
}

int message(string str) {
  if(peek_state()) {
    to_state_stack(str);
  } else {
    return send_string(str);
  }
}

int send_phrase(object obj) {
  /* This is how we'll control second, etc choice of locale later on. */

  return message(obj->to_string(this_object()));
}

int send_system_phrase(string phrname) {
  object phr;

  phr = PHRASED->file_phrase(SYSTEM_PHRASES, phrname);
  if(!phr) {
    LOGD->write_syslog("Can't find system phrase " + phrname + "!", LOG_ERR);
  }
  return send_phrase(phr);
}


/*
 * NAME:	receive_message()
 * DESCRIPTION:	process a message from the user
 */
int receive_message(string str)
{
  if (previous_program() == LIB_CONN) {
    if(peek_state()) {
      mixed tmp;

      tmp = state_receive_message(str);
      if(!peek_state()) {
	/* We started with a user_state active, but it's stopped now. */
	print_prompt();
      }
      return tmp;
    }

    return process_message(str);
  }
  error("receive_message called by illegal sender");
}


/*
 * NAME:	show_room_to_player()
 * DESCRIPTION:	give room desc to current player
 */
void show_room_to_player(object location) {
  string tmp;
  object phr;
  int    ctr;
  mixed* objs;

  if(!location) {
    send_system_phrase("you are nowhere");
    message("\r\n");
    return;
  }

  phr = location->get_brief();
  if(phr)
    tmp = phr->to_string(this_object());
  if(tmp) {
    message(tmp);
    message("\r\n");
  } else {
    send_system_phrase("(unnamed location)");
    message("\r\n");
  }

  phr = location->get_look();
  tmp = phr ? phr->to_string(this_object()) : nil;
  if(tmp) {
    message(tmp);
    message("\r\n");
  } else {
    send_system_phrase("(no room desc)");
    message("\r\n");
  }

  message("*****\r\n");

  objs = location->objects_in_container();
  for(ctr = 0; ctr < sizeof(objs); ctr++) {
    if(objs[ctr] != body) {
      message("- ");
      send_phrase(objs[ctr]->get_brief());
      message("\r\n");
    }
  }

  message("\r\n");

  if(function_object("num_exits", location)) {
    send_system_phrase("Exits");
    message(": ");
    for(ctr = 0; ctr < location->num_exits(); ctr++) {
      object exit;

      exit = location->get_exit_num(ctr);
      phr = EXITD->get_short_for_dir(exit->get_direction());
      message(phr->to_string(this_object()) + " ");
    }
    message("\r\n");
  }
}


private string* string_to_words(string str) {
  string *words;
  int     ctr;

  words = explode(str, " ");

  /* Trim */
  for(ctr = 0; ctr < sizeof(words); ctr++) {
    if(!words[ctr] || STRINGD->is_whitespace(words[ctr])) {
      words = words[..ctr-1] + words[ctr+1..];
    } else {
      words[ctr] = STRINGD->to_lower(STRINGD->trim_whitespace(words[ctr]));
    }
  }

  return words;
}


/* This function takes the supplied object list and searches it
   for objects that match the string.  This includes string matching,
   but also includes things like searching through open containers for any
   possible matches.  If only_details is true then objects will have
   their details searched, but not objects inside those details. */
private object* search_contained_objects(object* objs, string str,
					 varargs int only_details) {
  object *ret, *contents, *details, temp;
  string *words, err;
  int ctr, temp2;

  words = string_to_words(str);
  temp2 = 0;

  ret = ({ });
  if (sizeof(objs)) {
    temp = objs[0];
    temp2 = 1;
  }

  while(sizeof(objs)) {
    if(objs[0] == location
       || (!only_details
	   && objs[0]->is_container() && objs[0]->is_open())) {
      contents = objs[0]->objects_in_container();
      if(contents)
	objs += contents;
    }

    if(objs[0]->match_words(this_object(), words)) {
      ret += ({ objs[0] });
    }

    details = objs[0]->get_details();
    if(details && sizeof(details)) {
      objs += details;
    }

    objs = objs[1..];
  }

  return sizeof(ret) ? ret : nil;
}

private object* find_objects_in_loc(int loc, string str) {
  object *objs;
  int ctr;

  objs = ({ });

  if(!location &&
     (loc == LOC_CURRENT_ROOM || loc == LOC_IMMEDIATE_CURRENT_ROOM
      || loc == LOC_DETAIL_CURRENT_ROOM || LOC_CURRENT_EXITS))
    return nil;

  if(!body &&
     (loc == LOC_INVENTORY || loc == LOC_IMMEDIATE_INVENTORY
      || loc == LOC_BODY))
    return nil;

  switch(loc) {

  case LOC_IMMEDIATE_CURRENT_ROOM:
    return location->find_contained_objects(this_object(), str);

  case LOC_IMMEDIATE_INVENTORY:
    return body->find_contained_objects(this_object(), str);

  case LOC_CURRENT_ROOM:
    /* Pass location directly so its details will be searched */
    return search_contained_objects( ({ location }), str);

  case LOC_CURRENT_EXITS:
    /* Pass location directly so its details will be searched */
    if(function_object("num_exits", location)) {
      for(ctr = 0; ctr < location->num_exits(); ctr++) {
        objs += ({ location->get_exit_num(ctr) });
      }
    }
    return search_contained_objects( objs, str);

  case LOC_INVENTORY:
    /* Pass objects in body object so that body's details won't be
       searched */
    return search_contained_objects(body->objects_in_container(), str);

  case LOC_DETAIL_CURRENT_ROOM:
    return search_contained_objects( ({ location }), str, 1);

  case LOC_BODY:
    return search_contained_objects( ({ body }), str, 1);

  default:
    error("Unrecognized location (" + loc
	  + ") when finding objects!");
  }

  /* Never used */
  return nil;
}

object* find_objects(string str, int locations...) {
  int     ctr;
  object* objs;

  if(!SYSTEM() && !COMMON())
    error("Only System objects are allowed to call find_objects()!");

  objs = ({ });
  for(ctr = 0; ctr < sizeof(locations); ctr++) {
    objs += find_objects_in_loc(locations[ctr], str);
  }

  return sizeof(objs) ? objs : nil;
}


object* find_first_objects(string str, int locations...) {
  int     ctr;
  object* objs;

  if(!SYSTEM() && !COMMON())
    error("Only System objects are allowed to call find_first_objects()!");

  for(ctr = 0; ctr < sizeof(locations); ctr++) {
    objs = find_objects_in_loc(locations[ctr], str);
    if(objs && sizeof(objs)) {
      return objs;
    }
  }

  return nil;
}


void notify_moved(object obj) {
  if(previous_program() != USER_MOBILE) {
    error("Only MOBILEs can notify the User object that its body moved.");
  }

  location = body->get_location();
}



/*
 * Returns true if the name isn't allowed
 */
private int name_is_forbidden(string name) {
  string str;

  str = STRINGD->to_lower(name);

  if(str == "common")
    return 1;
  if(str == "system")
    return 1;
}


/*
 * NAME:	login()
 * DESCRIPTION:	login a new user
 */
int login(string str)
{
  if (previous_program() == LIB_CONN) {
    if(name_is_forbidden(str)) {
      previous_object()->message("\r\nThat name is forbidden.\r\n"
				 + "Please log in with a different one.\r\n");
      return MODE_DISCONNECT;
    }

    if (nconn == 0) {
      ::login(str);
    }
    nconn++;

    if (strlen(str) == 0 || sscanf(str, "%*s ") != 0 ||
	sscanf(str, "%*s/") != 0) {
      return MODE_DISCONNECT;
    }
    Name = name = str;
    if (Name[0] >= 'a' && Name[0] <= 'z') {
      Name[0] -= 'a' - 'A';
    }

    timestamp = time();
    hostname = query_ip_name(this_object());

    restore_user_from_file(str);

    if (password) {
      object phr;

      /* check password */
      phr = PHRASED->file_phrase(SYSTEM_PHRASES, "Password: ");
      previous_object()->message(phr->to_string(this_object()));

      set_state(previous_object(), STATE_LOGIN);
    } else {
      /* no password; login immediately */
      connection(previous_object());
      message_all_users(Name + " ");
      system_phrase_all_users("logs in.");
      message_all_users("\r\n");

      send_system_phrase("choose new password");
      set_state(previous_object(), STATE_NEWPASSWD1);

      /* Check if an immort */
      if (sizeof(rsrc::query_owners() & ({ str })) == 0) {
	return MODE_NOECHO;
      }

      /* If so, create a wiztool */
      if (!wiztool) {
	wiztool = clone_object(SYSTEM_WIZTOOL, str);
      }
    }
    return MODE_NOECHO;
  }

}


/*
 * NAME:	player_login()
 * DESCRIPTION:	Create the player body, set the account info and so on...
 */
private void player_login(void)
{
  int    start_room_num, start_zone;
  object start_room;

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
  if(!body) {
    location = start_room;

    body = clone_object(SIMPLE_ROOM);
    if(!body)
      error("Can't clone simple portable!");

    body->set_container(1);
    body->set_open(1);
    body->set_openable(0);

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

    mobile = clone_object(USER_MOBILE);
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


/*
 * NAME:	logout()
 * DESCRIPTION:	logout user
 */
void logout(int quit)
{
  if (previous_program() == LIB_CONN && --nconn == 0) {
    if (query_conn()) {
      if (quit) {
	message_all_users(Name + " ");
	system_phrase_all_users("logs out.");
	message_all_users("\r\n");
      } else {
	message_all_users(Name + " disconnected.\r\n");
      }
    }
    player_logout();
    ::logout();
    if (wiztool) {
      destruct_object(wiztool);
    }
    destruct_object(this_object());
  }
}


static int process_message(string str)
{
  string cmd;
  int    ctr, size;
  int    force_command;
  mixed* command;

  timestamp = time();

  switch (get_state(previous_object())) {
  case STATE_NORMAL:
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

      size = SYSTEM_USER->num_command_sets(locale);
      for(ctr = 0; ctr < size; ctr++) {
	command = SYSTEM_USER->query_command_sets(locale, ctr, cmd);
	if(command && sizeof(command) == 1) {
	  call_other(this_object(),                /* Call on self */
		     command[0],                   /* The function */
		     this_object(),                /* This user */
		     cmd,                          /* The command */
		     str == "" ? nil : str);       /* str or nil */
	  str = nil;
	  break;
	} else if(command && sizeof(command) == 2
		  && command[1] == "social") {
	  call_other(this_object(),                /* Call on self */
		     "cmd_social",                 /* Function cmd_social */
		     this_object(),                /* This user */
		     command[0],                   /* The command */
		     str == "" ? nil : str);       /* Extra arguments */
	  str = nil;
	  break;
	} if (command && sizeof(command)) {
	  LOGD->write_syslog("Unrecognized command format: "
			     + STRINGD->mixed_sprint(command), LOG_ERR);
	  break;
	}
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
    break;

  case STATE_LOGIN:
    if (crypt(str, password) != password) {
      object phr;

      previous_object()->message("\r\n");
      phr = PHRASED->file_phrase(SYSTEM_PHRASES, "Bad password.");
      previous_object()->message(phr->to_string(this_object()));
      previous_object()->message("\r\n");
      return MODE_DISCONNECT;
    }
    connection(previous_object());
    message("\r\n");
    message_all_users(Name + " ");
    system_phrase_all_users("logs in.");
    message_all_users("\r\n");
    if (!wiztool && sizeof(rsrc::query_owners() & ({ name })) != 0) {
      wiztool = clone_object(SYSTEM_WIZTOOL, name);
    }

    player_login();
    break;

  case STATE_OLDPASSWD:
    if (crypt(str, password) != password) {
      message("\r\n");
      send_system_phrase("Bad password.");
      message("\r\n");
      break;
    }
    message("\r\n");
    send_system_phrase("New password: ");
    set_state(previous_object(), STATE_NEWPASSWD1);
    return MODE_NOECHO;

  case STATE_NEWPASSWD1:
    if(strlen(str) == 0) {
      message("\r\n");
      send_system_phrase("Looks like no password");
      message("\r\n");
      if(password && strlen(password)) {
	send_system_phrase("Password change cancelled");
	return MODE_NOECHO;
      }

      return MODE_DISCONNECT;
    }
    if(strlen(str) < 4) {
      message("\r\n");
      send_system_phrase("must be four characters");
      message("\r\n");
      send_system_phrase("New password: ");
      return MODE_NOECHO;
    }
    newpasswd = str;
    message("\r\n");
    send_system_phrase("Retype new password: ");
    set_state(previous_object(), STATE_NEWPASSWD2);
    return MODE_NOECHO;

  case STATE_NEWPASSWD2:
    if (newpasswd == str) {
      password = crypt(str);
      save_user_to_file();
      message("\r\n");
      send_system_phrase("Password changed.");
      message("\r\n");
    } else {
      message("\r\n");
      send_system_phrase("Mismatch; password not changed.");
      message("\r\n");

      set_state(previous_object(), STATE_NEWPASSWD1);
      send_system_phrase("New password: ");
      return MODE_NOECHO;
    }
    newpasswd = nil;

    if(!location)
      player_login();

    break;
  }

  /* Don't print a prompt if we've pushed a state_stack -- they
     print their own prompts. */
  if(!peek_state()) {
    print_prompt();
  }

  set_state(previous_object(), STATE_NORMAL);
  return MODE_ECHO;
}

static void print_prompt(void) {
  string str;

  str = (wiztool) ? query_editor(wiztool) : nil;
  if (str) {
    message((str == "insert") ? "*\b" : ":");
  } else {
    message((name == "admin") ? "# " : "> ");
  }
}


/* Temporary command for testing the parser */

static void cmd_parse(object user, string cmd, string str) {
  mixed output;

  output = PARSED->parse_cmd(str);

  if (output == nil) {
    message("FAILED!\r\n");
  } else {
    message("PARSED!\r\n" + STRINGD->mixed_sprint(output) + "\r\n");
  }
}

/************** User-level commands *************************/

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

  system_phrase_all_users("(OOC)");
  message_all_users(" " + Name + " ");
  system_phrase_all_users("chats");
  message_all_users(": " + str + "\r\n");

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

static void cmd_help(object user, string cmd, string str) {
  mixed *hlp, *kw;
  int index;
  int exact;

  exact = 0;
  if (str && sscanf(str, "%d %s", index, str) == 2) {
    if(index < 1) {
      send_system_phrase("Usage: ");
      message(cmd + " <word>\r\n");
      message("   or  " + cmd + " <num> <word>\r\n");
      return;
    }
    exact = 1;
    index = index - 1;  /* User sees as 1-indexed, we see 0-indexed */
  } else if (!str || STRINGD->is_whitespace(str)) {

    if(wiztool) {
      str = "main_admin";
    } else {
      str = "main";
    }

    index = 0;
  } else if (str) {
    index = 0;
  }

  if(wiztool) {
    kw = ({ "admin" });
  } else {
    kw = ({ });
  }
  hlp = HELPD->query_exact_with_keywords(str, this_object(), kw);
  if(hlp) {
    if((exact && (sizeof(hlp) <= index)) || (sizeof(hlp) < 0)
       || (index < 0)) {
      message("Only " + sizeof(hlp) + " help files on \""
	      + str + "\".\r\n");
    } else {
      if(sizeof(hlp) > 1) {
	message("Help on " + str + ":    [" + sizeof(hlp) + " entries]\r\n");
      }
      message_scroll(hlp[index][1]->to_string(this_object()) + "\r\n");
    }
    return;
  }

  if(!exact) {
    string sdx_key;

    sdx_key = SOUNDEXD->get_key(str);
    hlp = HELPD->query_soundex_with_keywords(sdx_key, this_object(), ({ }));

    if(hlp && sizeof(hlp)) {
      if(index) {
	if(index < sizeof(hlp)) {
	  message("Help on " + hlp[index][0] + ":\r\n");
	  message(hlp[index][1]->to_string(this_object()));
	  message("\r\n");
	} else {
	  message("There are only " + sizeof(hlp)
		  + " help entries that sound like " + str + ".\r\n");
	}
      } else if(sizeof(hlp) == 1) {
	message_scroll("Help on " + hlp[0][0] + ":\r\n"
		       + hlp[0][1]->to_string(this_object())
		       + "\r\n");
      } else {
	message("\r\nWhich do you want help on:\r\n");
	for(index = 0; index < sizeof(hlp); index++) {
	  message("     " + hlp[index][0] + "\r\n");
	}
	message("(type \"help <topic>\" for the topic you want)\r\n");
      }
      return;
    }
  }

  message("No help on \"" + str + "\".\r\n");
}

static void cmd_tell(object self, string cmd, string str) {
  object user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2 ||
      !(user=user::find_user(username))) {
    send_system_phrase("Usage: ");
    message(cmd + " <user> <text>\r\n");
  } else {
    user->message(Name + " tells you: " + str + "\r\n");
  }
}

static void cmd_ask(object self, string cmd, string str) {
  object user;
  string username;

  if (sscanf(str, "%s %s", username, str) != 2 ||
      !(user=user::find_user(username))) {
    if (str == nil || strlen(str) == 0) {
      message("Usage: ask <text>\r\n" + 
	      "    or ask <user> <text>\r\n");
      return;
    } else {
      user = nil;
    }
  } else {
    /* for the moment ask just behaves like a whisper.
       This may change later */
    if(sizeof(explode(str, "?")) > 1)
      mobile->ask(user, str);
    else
      mobile->ask(user, str + "?");
  }
}

/* move to mobile (when implemented) */
static void cmd_yell(object user, string cmd, string str) {
  message("Unimplemented.  Use say or ooc.\r\n");
}

static void cmd_impbug(object user, string cmd, string str) {
  message("Unimplemented.  Use ooc.\r\n");
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


private void __sub_unsub_channels(object user, string cmd, int chan,
				  string channelname, string subval,
				  string sublevel) {
  /* Sub or unsub the user */
  if(!STRINGD->stricmp(subval, "on")
     || !STRINGD->stricmp(subval, "sub")
     || !STRINGD->stricmp(subval, "subscribe")) {

    if(sublevel) {
      int level;

      /* Subscribe with extra sub info */
      if((chan != CHANNEL_ERR && chan != CHANNEL_LOG)
	 || !wiztool) {
	user->message("You can't subscribe to any channels that use"
		      + " extra subscription info.\r\n");
	user->message("Usage: " + cmd + " <channel> [on|off]\r\n");
      }

      if(!sscanf(sublevel, "%d", level)) {
	/* Try and parse sublevel as a string */
	if(chan == CHANNEL_LOG || chan == CHANNEL_ERR
	   && LOGD->get_level_by_name(sublevel)) {
	  level = LOGD->get_level_by_name(sublevel);
	} else {
	  user->message("Not sure what level to use for '" + sublevel
			+ "'.\r\n");
	  return;
	}
      }

      /* Subscribe with extra info 'level' */
      if(CHANNELD->subscribe_user(user, chan, level) < 0) {
	  user->message("You can't subscribe to that channel.\r\n");
      } else {
	user->message("Subscribed to " + channelname + ", level " + level
		      + ".\r\n");
      }
    } else {

      /* Subscribe with no extra info */
      if(CHANNELD->subscribe_user(user, chan) < 0) {
	user->message("You can't subscribe to that channel.\r\n");
      } else {
	user->message("Subscribed to " + channelname + ".\r\n");
      }
    }

    /* Save new subscriptions, if any */
    save_user_to_file();

  } else if(!STRINGD->stricmp(subval, "off")
	    || !STRINGD->stricmp(subval, "unsub")
	    || !STRINGD->stricmp(subval, "unsubscribe")) {

    if(CHANNELD->unsubscribe_user(user, chan) < 0) {
      user->message("You can't unsub from that.  "
		    + "Are you sure you're subscribed?\r\n");
    } else {
      user->message("Unsubscribed from " + channelname + ".\r\n");
      save_user_to_file();
    }
  } else {
    user->message("Huh?  Try using 'on' or 'off' for the third value.\r\n");
  }
}

static void cmd_channels(object user, string cmd, string str) {
  int    ctr, chan;
  mixed* chanlist;
  string channelname, subval, sublevel;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    chanlist = CHANNELD->channel_list(user);
    user->message("Channels:\r\n");
    for(ctr = 0; ctr < sizeof(chanlist); ctr++) {
      if(CHANNELD->is_subscribed(user, ctr)) {
	user->message("* ");
      } else {
	user->message("  ");
      }
      user->send_phrase(chanlist[ctr][0]);

      if(chanlist[ctr][2] > 0) {
	user->message("  " + chanlist[ctr][2]);
      }

      user->message("\r\n");
    }
    user->message("-----\r\n");
    return;
  }

  if(!str || str == "" || sscanf(str, "%*s %*s %*s %*s") == 4) {
    if(wiztool) {
      send_system_phrase("Usage: ");
      message(cmd + " [<channel name> [on|off]] [extra info]\r\n");
    } else {
      send_system_phrase("Usage: ");
      message(cmd + " [<channel name> [on|off]]\r\n");
    }
    return;
  }

  if((sscanf(str, "%s %s %s", channelname, subval, sublevel) != 3)
     && (sscanf(str, "%s %s", channelname, subval) != 2)
     && (sscanf(str, "%s", channelname) != 1)) {
    user->message("Parsing error!\r\n");
    return;
  }

  chan = CHANNELD->get_channel_by_name(channelname, user);

  if(chan < 0) {
    user->message("You don't know any channel named '" + channelname
		  + "'.  Type 'channels' for a list of names.\r\n");
    return;
  }

  if(subval) {
    __sub_unsub_channels(user, cmd, chan, channelname, subval, sublevel);
    return;
  }

  /* Check whether you're subbed and whether the channel is available
     here. */
  user->message("Channel: " + channelname + "\r\n");
  if(CHANNELD->is_subscribed(user, chan)) {
    user->message("You are currently subscribed to that channel.\r\n");
  } else {
    user->message("You are not currently subscribed to that channel.\r\n");
  }
  user->message("That channel is available in this area.\r\n");
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
