/* $Header: /cvsroot/phantasmal/mudlib/usr/System/obj/user.c,v 1.75 2003/12/08 09:07:32 angelbob Exp $ */

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

/***********************************************************************/
/* THIS USER OBJECT SHOULD *ONLY* EVER BE USED BY TELNETD WHEN *NO*    */
/* REGULAR USER OBJECT CAN BE FOUND.  IT IS NOT MEANT FOR NORMAL USE,  */
/* NOR SHOULD IT EVER HAVE A BODY OR REASONABLE CAPABILITIES.  FOR THE */
/* REAL USER OBJECT, SEE /usr/game/obj/user.c OR THE EQUIVALENT IN     */
/* YOUR GAME OF CHOICE.                                                */
/***********************************************************************/

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
		     "ooc"       : "cmd_ooc",
		     "help"      : "cmd_help",
		     "whoami"    : "cmd_whoami",
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

  /* Set the start room to the void */
  start_room_num = 0;
  start_room = MAPD->get_room_by_num(start_room_num);
  start_zone = 0;
  if(start_room == nil) {
    /* Panic!  No void! */
    error("Internal Error: no Void!");
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

    if(sscanf(cmd, "%s %s", cmd, str) != 2) {
      str = "";
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
  message("No /usr/game/obj/user exists!  Use ooc cmd to complain.\r\n");
  return -1;
}


/************** User-level commands *************************/

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

static void cmd_whoami(object user, string cmd, string str) {
  message("You are '" + name + "'.\r\n");
}
