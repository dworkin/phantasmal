/* $Header: /cvsroot/phantasmal/mudlib/usr/System/obj/user.c,v 1.12 2002/06/06 18:54:08 angelbob Exp $ */

#include <kernel/kernel.h>
#include <kernel/user.h>
#include <kernel/rsrc.h>
#include <config.h>
#include <type.h>
#include <log.h>
#include <phrase.h>
#include <channel.h>

inherit LIB_USER;
inherit user API_USER;
inherit rsrc API_RSRC;

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
int    body_num;                /* portable number of body */

/* User-state processing stack */
private object* state_stack;
private mixed   state_data;

/* Commandset processing */
private mixed* command_sets;

/* Random unsaved */
static string Name;		/* capitalized user name */
static mapping state;		/* state for a connection object */
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
void upgraded(void);
static  int    process_message(string str);
static  void   print_prompt(void);
private int    name_is_forbidden(string name);
private void   tell_room(object room, mixed msg);
private mixed* load_command_sets_file(string filename);

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
    state = ([ ]);

    state_stack = ({ });

    /* Default to enUS locale */
    locale = PHRASED->language_by_name("english");
    command_sets = nil;
  } else {
    upgraded();
  }
}

void upgraded(void) {
  if(!find_object(SYSTEM_WIZTOOL)) { compile_object(SYSTEM_WIZTOOL); }
  if(!find_object(SIMPLE_MOBILE)) { compile_object(SIMPLE_MOBILE); }
  if(!find_object(SIMPLE_PORTABLE)) { compile_object(SIMPLE_PORTABLE); }

  command_sets = load_command_sets_file(USER_COMMANDS_FILE);
  if(!command_sets) {
    LOGD->write_syslog("Command_sets is Nil!", LOG_FATAL);
  }
}


int num_command_sets(int loc) {
  if(!command_sets[loc])
    return 0;

  return sizeof(command_sets[loc]);
}

mixed* query_command_sets(int loc, int num, string cmd) {
  mixed* ret;

  ret = command_sets[loc][num][cmd];

  if(ret) {
    return ret[..];
  } else {
    return nil;
  }
}

private void add_commands_to_set(mixed* tmp_cmd, int loc, string cmds) {
  mixed* entries;
  int    ctr;
  string cmd, func;

  if(!tmp_cmd[loc]) {
    tmp_cmd[loc] = ({ ([ ]) });
  }
  if(!sizeof(tmp_cmd[loc])) {
    tmp_cmd[loc] += ({ ([ ]) });
  }
  if(!tmp_cmd[loc][0]) {
    tmp_cmd[loc][0] = ([ ]);
  }

  entries = explode(cmds, ",");
  for(ctr = 0; ctr < sizeof(entries); ctr++) {
    entries[ctr] = STRINGD->trim_whitespace(entries[ctr]);
    if(sscanf(entries[ctr], "%s/%s", cmd, func) != 2) {
      error("Command entry doesn't match format: '" + entries[ctr]
	    + "'");
    }
    cmd = STRINGD->trim_whitespace(cmd);
    func = STRINGD->trim_whitespace(func);
    tmp_cmd[loc][0][cmd] = ({ func });
  }
}

private mixed* load_command_sets_file(string filename) {
  mixed*  tmp_cmd, *unq;
  string  file;
  int     ctr, err, loc;

  file = read_file(filename);
  if(!file) {
    LOGD->write_syslog("Couldn't find file " + filename
		       + ", not updating command_sets!", LOG_ERR);
    return command_sets;
  }

  if(!find_object(PHRASED)) { compile_object(PHRASED); }
  LOGD->write_syslog("Allocating command_sets", LOG_NORMAL);
  tmp_cmd = allocate(PHRASED->num_locales());

  unq = UNQ_PARSER->basic_unq_parse(file);
  if(!unq) {
    LOGD->write_syslog("Couldn't parse file " + filename
		       + " as UNQ!, not updating command_sets!", LOG_ERR);
    return command_sets;
  }

  err = 0;
  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    /* Filter out whitespace */
    if(!unq[ctr] || STRINGD->is_whitespace(unq[ctr])) {
      if(typeof(unq[ctr + 1]) != T_STRING
	 || !STRINGD->is_whitespace(unq[ctr + 1])) {
	LOGD->write_syslog("Error: not type T_STRING", LOG_ERR);
	err = 1;
	break;
      }
      continue;
    }
    if(typeof(unq[ctr + 1]) != T_STRING) {
      LOGD->write_syslog("Error: not type T_STRING: '"
			 + STRINGD->mixed_sprint(unq[ctr + 1]) + "'/"
			 + typeof(unq[ctr + 1]), LOG_ERR);
      err = 1;
      break;
    }

    /* Trim whitespace in remaining entries */
    unq[ctr] = STRINGD->trim_whitespace(unq[ctr]);
    unq[ctr+1] = STRINGD->trim_whitespace(unq[ctr+1]);

    loc = PHRASED->language_by_name(unq[ctr]);
    if(loc == -1) {
      LOGD->write_syslog("Error: unknown locale '" + unq[ctr] + "'", LOG_ERR);
      err = 1;
      break;
    }

    add_commands_to_set(tmp_cmd, loc, unq[ctr + 1]);
  }
  if(err) {
    LOGD->write_syslog("Error decoding UNQ, not updating command_sets!",
		       LOG_ERR);
    return command_sets;
  }

  return tmp_cmd;
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

object get_location(void) {
  return location;
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


/****** USER_STATE stack implementation ********/

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

int message(string str) {
  if(state_stack && sizeof(state_stack)) {
    state_stack[0]->to_user(str);
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

void pop_state(object state) {
  int first_state;

  if(!state_stack || !sizeof(state_stack))
    error("Popping empty stack!");

  if(!(state_stack && ({ state })))
    error("Popping state not in stack!");

  if(state_stack[0] == state)
    first_state = 1;
  else
    first_state = 0;

  state_stack = state_stack - ({ state });

  if(first_state) {
    state->switch_from(1);  /* 1 because popp is true */
    if(sizeof(state_stack)) {
      state_stack[0]->switch_to(0);  /* 0 because pushp is false */
    } else {
      print_prompt();
    }
  }

  destruct_object(state);
}

void user_state_data(mixed data) {
  if(data == nil) {
    /* Passing nil means "done now, print a prompt". */
    print_prompt();
    return;
  }
  state_data = data;
}

void push_state(object state) {
  if(!state_stack) {
    state_stack = ({ });
  }

  if(sizeof(state_stack)) {
    state->init(this_object(), state_stack[0]);
    state_stack[0]->switch_from(0);  /* 0 because popp is false */
  } else {
    state->init(this_object(), nil);
  }
  state->switch_to(1); /* 1 because pushp is true */

  state_stack = ({ state }) + state_stack;
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

  chanlist = CHANNELD->channel_list(is_admin());
  subcode = 0;
  for(ctr = 0; ctr < sizeof(chanlist); ctr++) {
    if(CHANNELD->is_subscribed(this_object(), chanlist[ctr][1])) {
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
      subcode -= chan_code;
      if(CHANNELD->subscribe_user(this_object(), channel, "") < 0) {
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


/*
 * NAME:	message_all_users()
 * DESCRIPTION:	send message to listening users
 */
private void message_all_users(string str)
{
    object *users, user;
    int i;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
	user = users[i];
	if (user != this_object() &&
	    sscanf(object_name(user), SYSTEM_USER + "#%*d") != 0) {
	    user->message(str);
	}
    }
}


/*
 * NAME:	system_phrase_all_users()
 * DESCRIPTION:	send message to listening users
 */
private void system_phrase_all_users(string str)
{
    object *users, user;
    int i;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
	user = users[i];
	if (user != this_object() &&
	    sscanf(object_name(user), SYSTEM_USER + "#%*d") != 0) {
	    user->send_system_phrase(str);
	}
    }
}


private void system_phrase_room(object room, string str)
{
  object phr;

  phr = PHRASED->file_phrase(SYSTEM_PHRASES, str);
  if(!phr)
    error("Can't get system phrase " + str);

  tell_room(room, phr);
}

/*
 * NAME:	tell_room()
 * DESCRIPTION:	send message to everybody in specified location
 */
private void tell_room(object room, mixed msg)
{
  object *mobiles, mobile, user;
  int i;

  if(!room) {
    LOGD->write_syslog("Tell_room called on location nil!", LOG_WARN);
    return;
  }
  mobiles = room->mobiles_in_container();
  for (i = sizeof(mobiles); --i >= 0; ) {
    mobile = mobiles[i];
    user = mobile->get_user();
    if (user && user != this_object() &&
	sscanf(object_name(user), SYSTEM_USER + "#%*d") != 0) {
      typeof(msg) == T_STRING ?
	user->message(msg)
	: user->send_phrase(msg);
    }
  }
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
    if(!objs[ctr]->is_no_desc()) {
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


void move_player(object room) {
  if(location) {
    location->remove_from_container(body);
  }

  if(room) {
    room->add_to_container(body);
  }
}


void notify_moved(object obj) {
  if(previous_program() != MOBILE) {
    error("Only MOBILEs can notify the User object that its body moved.");
  }

  location = body->get_location();
}


private object* find_object_in_room(object user, object room, string namestr) {
  string  noun, *words;
  object* ret;
  mixed*  objs;
  int     ctr;

  words = explode(namestr, " ");

  /* Trim */
  for(ctr = 0; ctr < sizeof(words); ctr++) {
    if(!words[ctr] || STRINGD->is_whitespace(words[ctr])) {
      words = words[..ctr-1] + words[ctr+1..];
    } else {
      words[ctr] = STRINGD->to_lower(STRINGD->trim_whitespace(words[ctr]));
    }
  }

  if(sizeof(words) == 0)
    return nil;

  ret = ({ });
  noun = words[sizeof(words) - 1];
  words = words[..sizeof(words)-2];  /* words is now a list of adjectives */

  objs = room->objects_in_container();
  for(ctr = 0; ctr < sizeof(objs); ctr++) {
    if(objs[ctr]->match_description(user, words, ({ noun }))) {
      ret += ({ objs[ctr] });
    }
  }

  if(sizeof(ret))
    return ret;

  return nil;
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

      state[previous_object()] = STATE_LOGIN;
    } else {
      /* no password; login immediately */
      connection(previous_object());
      message_all_users(Name + " ");
      system_phrase_all_users("logs in.");
      message_all_users("\r\n");

      send_system_phrase("choose new password");
      state[previous_object()] = STATE_NEWPASSWD1;

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
  int    start_room_num;
  object start_room;

  body = nil;

  /* Set up location, body, etc */
  start_room_num = CONFIGD->get_start_room();
  start_room = MAPD->get_room_by_num(start_room_num);

  if(body_num > 0) {
    body = PORTABLED->get_portable_by_num(body_num);
  }
  if(!body) {
    location = start_room;

    body = clone_object(SIMPLE_PORTABLE);
    if(!body)
      error("Can't clone simple portable!");
    PORTABLED->add_portable_number(body, -1);
    if(!PORTABLED->get_portable_by_num(body->get_number())) {
      LOGD->write_syslog("Error making new body!", LOG_ERR);
    }
    body_num = body->get_number();
    LOGD->write_syslog("Set body to number " + body_num + ".");

    if(!PORTABLED->get_portable_by_num(body_num)) {
      LOGD->write_syslog("Can't find new body number!", LOG_ERR);
    }

    /* Set descriptions and add noun for new name */
    body->set_brief(NEW_PHRASE(Name));
    body->set_glance(NEW_PHRASE(Name));
    body->set_look(NEW_PHRASE(Name + " wanders the MUD."));
    body->set_examine(nil);
    body->add_noun(NEW_PHRASE(name));

    mobile = clone_object(SIMPLE_MOBILE);
    mobile->assign_body(body);
    mobile->set_user(this_object());

    location->add_to_container(body);

    /* We just set a body number, so we need to save the player data
       file again... */
    save_user_to_file();
  } else {
    /* Move body to start room? */

    location = body->get_location();
    mobile = body->get_mobile();
    mobile->set_user(this_object());
    if(!mobile) {
      /* Deleted mobile but not body -- fix later */
      error("Body but not mobile deleted!  Deleting body, log in again.");
      message("Internal error, try logging in again.");
      destruct_object(body);
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
  /* TODO: Teleport body to meat locker */

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



/*
 * NAME:	receive_message()
 * DESCRIPTION:	process a message from the user
 */
int receive_message(string str)
{
  if (previous_program() == LIB_CONN) {
    if(state_stack && sizeof(state_stack)) {
      return state_stack[0]->from_user(str);
    }

    return process_message(str);
  }
  error("receive_message called by illegal sender");
}


static int process_message(string str)
{
  string cmd;
  int    ctr, size;
  int    force_command;
  mixed* command;

  timestamp = time();

  switch (state[previous_object()]) {
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
	    state[previous_object()] = STATE_OLDPASSWD;
	  } else {
	    send_system_phrase("New password: ");
	    state[previous_object()] = STATE_NEWPASSWD1;
	  }
	  return MODE_NOECHO;

	case "quit":
	  return MODE_DISCONNECT;
	}
      }

      size = SYSTEM_USER->num_command_sets(locale);
      for(ctr = 0; ctr < size; ctr++) {
	command = SYSTEM_USER->query_command_sets(locale, ctr, cmd);
	if(command) {
	  call_other(this_object(),                /* Call on self... */
		     command[0],                   /* The function */
		     this_object(),                /* This user */
		     cmd,                          /* The command */
		     str == "" ? nil : str);       /* str or nil */
	  str = nil;
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
    state[previous_object()] = STATE_NEWPASSWD1;
    return MODE_NOECHO;

  case STATE_NEWPASSWD1:
    if(strlen(str) == 0) {
      message("\r\n");
      send_system_phrase("Looks like no password");
      message("\r\n");
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
    state[previous_object()] = STATE_NEWPASSWD2;
    return MODE_NOECHO;

  case STATE_NEWPASSWD2:
    if (newpasswd == str) {
      password = crypt(str);
      /* save_object(SYSTEM_USER_DIR + "/" + name + ".pwd"); */
      save_user_to_file();
      message("\r\n");
      send_system_phrase("Password changed.");
      message("\r\n");
    } else {
      message("\r\n");
      send_system_phrase("Mismatch; password not changed.");
      message("\r\n");
    }
    newpasswd = nil;

    if(!location)
      player_login();

    break;
  }

  /* Don't print a prompt if we've pushed a state_stack -- they
     print their own prompts. */
  if(!state_stack || !sizeof(state_stack)) {
    print_prompt();
  }

  state[previous_object()] = STATE_NORMAL;
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


/************** User-level commands *************************/

static void cmd_ooc(object user, string cmd, string str) {
  if (!str || str == "") {
    send_system_phrase("Usage: ");
    message(cmd + " <text>\r\n");
  } else {
    system_phrase_all_users("(OOC)");
    message_all_users(" " + Name + " ");
    system_phrase_all_users("chats");
    message_all_users(": " + str + "\r\n");

    send_system_phrase("(OOC)");
    message(" ");
    send_system_phrase("You chat");
    message(": " + str + "\r\n");
  }
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

  tell_room(location, Name + " ");
  system_phrase_room(location, "says");
  tell_room(location, ": " + str + "\r\n");

  send_system_phrase("You say");
  message(": " + str + "\r\n");

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

  tell_room(location, Name + " " + str + "\r\n");

  message(Name + " " + str + "\r\n");
}

static void cmd_help(object user, string cmd, string str) {
  mixed *hlp;
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
    str = "main";
    index = 0;
  } else if (str) {
    index = 0;
  }

  hlp = HELPD->query_exact_with_keywords(str, this_object(), ({ }));
  if(hlp) {
    if((exact && (sizeof(hlp) <= index)) || (sizeof(hlp) < 0)
       || (index < 0)) {
      message("Only " + sizeof(hlp) + " help files on \""
	      + str + "\".\r\n");
    } else {
      if(sizeof(hlp) > 1) {
	message("Help on " + str + ":    [" + sizeof(hlp) + " entries]\r\n");
      }
      message(hlp[index][1]->to_string(this_object()));
      message("\r\n");
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
	message("Help on " + hlp[0][0] + ":\r\n");
	message(hlp[0][1]->to_string(this_object()));
	message("\r\n");
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
    send_system_phrase("Usage: ");
    message(cmd + " <user> <text>\r\n");
  } else {
    if(sizeof(explode(str, "?")) > 1)
      user->message(Name + " asks you: " + str + "\r\n");
    else
      user->message(Name + " asks you: " + str + "?\r\n");
  }
}

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

  if(sscanf(str, "in %s", str) || sscanf(str, "inside %s", str)
     || sscanf(str, "within %s", str) || sscanf(str, "into %s", str)) {
    /* Look inside container */
    str = STRINGD->trim_whitespace(str);
    tmp = find_object_in_room(user, body, str);
    if(!tmp) {
      tmp = find_object_in_room(user, location, str);
    }
    if(!tmp) {
      user->message("You don't find any '" + str + "'.\r\n");
      return;
    }
    if(sizeof(tmp) > 1) {
      user->message("You see more than one '" + str +"'.  You pick one.\r\n");
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

  tmp = find_object_in_room(user, body, str);
  if(!tmp || !sizeof(tmp)) {
    tmp = find_object_in_room(user, location, str);
    if(!tmp || !sizeof(tmp)) {
      user->message("You don't find any '" + str + "'.\r\n");
      return;
    }
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

  if(sscanf(str, "%s inside %s", obj1, obj2) != 2
     && sscanf(str, "%s into %s", obj1, obj2) != 2
     && sscanf(str, "%s in %s", obj1, obj2) != 2) {
    user->message("Usage: " + cmd + " <obj1> in <obj2>\r\n");
    return;
  }

  portlist = find_object_in_room(user, body, obj1);
  if(!portlist || !sizeof(portlist)) {
    portlist = find_object_in_room(user, location, obj1);
    if(!portlist || !sizeof(portlist)) {
      user->message("You can't find any '" + obj1 + "' here.\r\n");
      return;
    }
  }

  contlist = find_object_in_room(user, body, obj2);
  if(!contlist || !sizeof(contlist)) {
    contlist = find_object_in_room(user, location, obj2);
    if(!contlist || !sizeof(contlist)) {
      user->message("You can't find any '" + obj2 + "' here.\r\n");
      return;
    }
  }

  tmp = ({ });
  for(ctr = 0; ctr < sizeof(contlist); ctr++) {
    if(contlist[ctr]->is_container()
       && contlist[ctr]->is_open()) {
      tmp += ({ contlist[ctr] });
    }
  }
  contlist = tmp;
  tmp = nil;

  if(!sizeof(contlist)) {
    user->message("Nothing by the name '" + obj2
		  + "' is an open container.\r\n");
    return;
  }

  tmp = ({ });
  for(ctr = 0; ctr < sizeof(portlist); ctr++) {
    if(PORTABLED->get_portable_by_num(portlist[ctr]->get_number())
       && !portlist[ctr]->is_no_desc()) {
      tmp += ({ portlist[ctr] });
    }
  }
  portlist = tmp;
  tmp = nil;

  if(!sizeof(portlist)) {
    user->message("Nothing by the name '" + obj2
		  + "' can be put into an open container.\r\n");
    return;
  }

  if(sizeof(portlist) > 1) {
    user->message("More than one object fits '" + obj1 + "'.  "
		  + "You pick one.\r\n");
  }

  if(sizeof(contlist) > 1) {
    user->message("More than one open container fits '" + obj2 + "'.  "
		  + "You pick one.\r\n");
  }

  port = portlist[0];
  cont = contlist[0];

  if(port->get_location()) {
    port->get_location()->remove_from_container(port);
  }
  cont->add_to_container(port);

  user->message("You put ");
  user->send_phrase(port->get_brief());
  user->message(" in ");
  user->send_phrase(cont->get_brief());
  user->message(".\r\n");
}

static void cmd_remove(object user, string cmd, string str) {
  string  obj1, obj2;
  object* portlist, *contlist, *tmp;
  object  port, cont;
  int     ctr;

  if(sscanf(str, "%s from inside %s", obj1, obj2) != 2
     && sscanf(str, "%s from in %s", obj1, obj2) != 2
     && sscanf(str, "%s from %s", obj1, obj2) != 2
     && sscanf(str, "%s out of %s", obj1, obj2) != 2) {
    user->message("Usage: " + cmd + " <obj1> from <obj2>\r\n");
    return;
  }

  contlist = find_object_in_room(user, body, obj2);
  if(!contlist || !sizeof(contlist)) {
    contlist = find_object_in_room(user, location, obj2);
    if(!contlist || !sizeof(contlist)) {
      user->message("You can't find any '" + obj2 + "' here.\r\n");
      return;
    }
  }

  tmp = ({ });
  for(ctr = 0; ctr < sizeof(contlist); ctr++) {
    if(contlist[ctr]->is_container()
       && contlist[ctr]->is_open()) {
      tmp += ({ contlist[ctr] });
    }
  }
  contlist = tmp;
  tmp = nil;
  if(!contlist || !sizeof(contlist)) {
    user->message("Nothing matching '" + obj2
		  + "' is an open container.\r\n");
    return;
  }

  if(sizeof(contlist) > 1) {
    user->message("More than one open container fits '" + obj2 + "'.\r\n");
    user->message("You pick one.\r\n");
  }
  cont = contlist[0];

  portlist = find_object_in_room(user, cont, obj1);
  if(!portlist || !sizeof(portlist)) {
    user->message("You can't find any '" + obj1 + "' in ");
    user->send_phrase(cont->get_brief());
    user->message(".\r\n");
    return;
  }

  tmp = ({ });
  while(sizeof(portlist)) {
    if(!portlist[0]->is_no_desc()
       && PORTABLED->get_portable_by_num(portlist[0]->get_number())) {
      tmp += ({ portlist[0] });
    }
    portlist = portlist[1..];
  }
  portlist = tmp;
  if(sizeof(portlist) == 0) {
    message("You can't move that around!\r\n");
    return;
  }

  if(sizeof(portlist) > 1) {
    user->message("More than one object fits '" + obj1 + "'.\r\n");
    user->message("You pick one.\r\n");
  }
  port = portlist[0];

  if(port->get_location()) {
    port->get_location()->remove_from_container(port);
  }
  body->add_to_container(port);

  user->message("You " + cmd + " ");
  user->send_phrase(port->get_brief());
  user->message(" from ");
  user->send_phrase(cont->get_brief());
  user->message(" (taken).\r\n");
}

static void cmd_bug(object user, string cmd, string str) {
  write_file(BUG_DATA, ctime(time()) + ": Bug Report: " + str + "\n");
  message("Reported bug: " + str + "\r\n");
}

static void cmd_typo(object user, string cmd, string str) {
  write_file(TYPO_DATA, ctime(time()) + ": Bug Report: " + str + "\n\n");
  message("Reported typo: " + str + "\r\n");
}

static void cmd_movement(object user, string cmd, string str) {
  int    dir;
  object opp_name;
  object exit, dest, loc;

  /* Currently, we ignore modifiers (str) and just move */

  dir = EXITD->direction_by_string(cmd);
  if(dir == -1) {
    user->message("'" + cmd + "' doesn't look like a valid direction.\r\n");
    return;
  }

  exit = location->get_exit(dir);
  if(!exit) {
    user->message("You don't see an exit in that direction.\r\n");
    return;
  }

  loc = location;
  dest = exit->get_destination();
  if(!dest) {
    user->message("That exit appears to disappear through a rift and vanish!");
    return;
  }

  opp_name = EXITD->get_name_for_dir(EXITD->opposite_direction(dir));
  tell_room(dest, Name + " enters from the ");
  tell_room(dest, opp_name);
  tell_room(dest, ".\r\n");

  move_player(dest);

  tell_room(loc, Name + " leaves going ");
  tell_room(loc, EXITD->get_name_for_dir(dir));
  tell_room(loc, ".\r\n");

  show_room_to_player(dest);
}


static void cmd_gossip(object user, string cmd, string str) {
  str = STRINGD->trim_whitespace(str);

  if(!str || str == "") {
    send_system_phrase("Usage: ");
    message(cmd + " <message>\r\n");
    return;
  }

  CHANNELD->string_to_channel(CHANNEL_GOSSIP, Name);
  CHANNELD->phrase_to_channel(CHANNEL_GOSSIP, NEW_PHRASE(" gossips "));
  CHANNELD->string_to_channel(CHANNEL_GOSSIP, "'" + str + "'.\r\n");
}


static void cmd_channels(object user, string cmd, string str) {
  int    ctr, chan;
  mixed* chanlist;
  string channelname, subval;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    chanlist = CHANNELD->channel_list(user->is_admin());
    user->message("Channels:\r\n");
    for(ctr = 0; ctr < sizeof(chanlist); ctr++) {
      if(CHANNELD->is_subscribed(user, ctr)) {
	user->message("* ");
      } else {
	user->message("  ");
      }
      user->send_phrase(chanlist[ctr][0]);

      user->message("\r\n");
    }
    user->message("-----\r\n");
    return;
  }

  if(!str || str == "" || sscanf(str, "%*s %*s %*s") == 3) {
    send_system_phrase("Usage: ");
    message(cmd + " [<channel name> [on|off]]\r\n");
    return;
  }

  if((sscanf(str, "%s %s", channelname, subval) != 2)
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
    /* Sub or unsub the user */
    if(!STRINGD->stricmp(subval, "on")
       || !STRINGD->stricmp(subval, "sub")
       || !STRINGD->stricmp(subval, "subscribe")) {

      if(CHANNELD->subscribe_user(user, chan, "") < 0) {
	user->message("You can't subscribe to that channel.\r\n");
      } else {
	user->message("Subscribed to " + channelname + ".\r\n");
	save_user_to_file();
      }
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
      return;
    }

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
  object* tmp, *tmp2;

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

  tmp = find_object_in_room(user, location, str);
  if(!tmp || !sizeof(tmp)) {
    message("You don't find any '" + str + "'.\r\n");
    return;
  }

  tmp2 = ({ });
  while(sizeof(tmp)) {
    if(!tmp[0]->is_no_desc()
       && PORTABLED->get_portable_by_num(tmp[0]->get_number())) {
      tmp2 += ({ tmp[0] });
    }
    tmp = tmp[1..];
  }

  if(sizeof(tmp2) == 0) {
    message("You can't carry that!\r\n");
    return;
  }

  if(sizeof(tmp2) > 1) {
    message("More than one of those is here.  You choose one.\r\n");
  }

  tmp2[0]->get_location()->remove_from_container(tmp2[0]);
  body->add_to_container(tmp2[0]);
  message("You " + cmd + " ");
  send_phrase(tmp2[0]->get_glance());
  message(".\r\n");
}


static void cmd_drop(object user, string cmd, string str) {
  object* tmp;

  if(str)
    str = STRINGD->trim_whitespace(str);
  if(!str || str == "") {
    message("Usage: " + cmd + " <description>\r\n");
    return;
  }

  tmp = find_object_in_room(user, body, str);
  if(!tmp || !sizeof(tmp)) {
    message("You're not carrying any '" + str + "'.\r\n");
    return;
  }

  if(sizeof(tmp) > 1) {
    message("You have more than one of those.  You drop one.\r\n");
  }

  body->remove_from_container(tmp[0]);
  location->add_to_container(tmp[0]);
  message("You drop ");
  send_phrase(tmp[0]->get_glance());
  message(".\r\n");
}
