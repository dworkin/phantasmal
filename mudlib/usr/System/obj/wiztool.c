#include <kernel/kernel.h>
#include <kernel/user.h>
#include <config.h>
#include <type.h>

inherit SYSTEM_WIZTOOLLIB;

private object user;		/* associated user object */
private string owner;
private string directory;
private object driver;

private mixed* command_sets;

private object room_dtd;        /* DTD for room def'n */
private object port_dtd;        /* DTD for portable def'n */


/* Prototypes */
void upgraded(void);


/*
 * NAME:	create()
 * DESCRIPTION:	initialize object
 */
static void create(int clone)
{
  ::create(clone);
  if (clone) {
    user = this_user();
    command_sets = nil;
  } else {
    upgraded();
  }
}

void destructed(int clone) {
  if(!clone && room_dtd) {
    destruct_object(room_dtd);
  }
  if(!clone && port_dtd) {
    destruct_object(port_dtd);
  }
  ::destructed(clone);
}

/* Called by objectd when recompiling */
void upgraded(void) {
  string dtd_file;

  command_sets
    = ({
      ([
	"@set_glance"          : ({ "cmd_set_obj_desc" }),
	"@set_brief"           : ({ "cmd_set_obj_desc" }),
	"@set_look"            : ({ "cmd_set_obj_desc" }),
	"@set_examine"         : ({ "cmd_set_obj_desc" }),
	"@stat"                : ({ "cmd_stat" }),
	"@set_obj_art"         : ({ "cmd_set_obj_article" }),
	"@set_object_art"      : ({ "cmd_set_obj_article" }),
	"@set_obj_article"     : ({ "cmd_set_obj_article" }),
	"@set_object_article"  : ({ "cmd_set_obj_article" }),
	"@set_obj_par"         : ({ "cmd_set_obj_parent" }),
	"@set_obj_parent"      : ({ "cmd_set_obj_parent" }),
	"@set_object_par"      : ({ "cmd_set_obj_parent" }),
	"@set_object_parent"   : ({ "cmd_set_obj_parent" }),
	"@set_ur_object"       : ({ "cmd_set_obj_parent" }),
	"@set_parent"          : ({ "cmd_set_obj_parent" }),
	"@add_noun"            : ({ "cmd_add_nouns" }),
	"@add_nouns"           : ({ "cmd_add_nouns" }),
	"@clear_nouns"         : ({ "cmd_clear_nouns" }),
	"@add_adjective"       : ({ "cmd_add_adjectives" }),
	"@add_adjectives"      : ({ "cmd_add_adjectives" }),
	"@add_adj"             : ({ "cmd_add_adjectives" }),
	"@clear_adjectives"    : ({ "cmd_clear_adjectives" }),
	"@clear_adj"           : ({ "cmd_clear_adjectives" }),
	"@move"                : ({ "cmd_move_obj" }),
	"@move_obj"            : ({ "cmd_move_obj" }),

	"@list_room"           : ({ "cmd_list_room" }),
	"@list_rooms"          : ({ "cmd_list_room" }),
	"@goto"                : ({ "cmd_goto_room" }),
	"@goto_room"           : ({ "cmd_goto_room" }),
	"@save_room"           : ({ "cmd_save_rooms" }),
	"@save_rooms"          : ({ "cmd_save_rooms" }),
	"@load_room"           : ({ "cmd_load_rooms" }),
	"@load_rooms"          : ({ "cmd_load_rooms" }),
	"@new_room"            : ({ "cmd_new_room" }),
	"@add_room"            : ({ "cmd_new_room" }),
	"@delete_room"         : ({ "cmd_delete_room" }),

	"@new_exit"            : ({ "cmd_new_exit" }),
	"@clear_exits"         : ({ "cmd_clear_exits" }),
	"@clear_exit"          : ({ "cmd_clear_exits" }),
	"@remove_exit"         : ({ "cmd_remove_exit" }),
	"@list_exit"           : ({ "cmd_list_exit" }),
	"@list_exits"          : ({ "cmd_list_exit" }),
	"@add_deferred_exits"  : ({ "cmd_add_deferred_exits" }),
	"@add_deferred"        : ({ "cmd_add_deferred_exits" }),
	"@check_deferred"      : ({ "cmd_check_deferred_exits" }),

	"@new_port"            : ({ "cmd_new_portable" }),
	"@new_portable"        : ({ "cmd_new_portable" }),
	"@add_port"            : ({ "cmd_new_portable" }),
	"@add_portable"        : ({ "cmd_new_portable" }),
	"@delete_port"         : ({ "cmd_delete_portable" }),
	"@delete_portable"     : ({ "cmd_delete_portable" }),
	"@list_port"           : ({ "cmd_list_portables" }),
	"@list_portable"       : ({ "cmd_list_portables" }),
	"@list_portables"      : ({ "cmd_list_portables" }),
	"@save_portables"      : ({ "cmd_save_portables" }),
	"@save_portable"       : ({ "cmd_save_portables" }),
	"@save_port"           : ({ "cmd_save_portables" }),
	"@load_portables"      : ({ "cmd_load_portables" }),
	"@load_portable"       : ({ "cmd_load_portables" }),
	"@load_port"           : ({ "cmd_load_portables" }),
	"@set_port_flag"       : ({ "cmd_set_port_flag" }),
	"@set_port_flags"      : ({ "cmd_set_port_flag" }),
	"@set_portable_flag"   : ({ "cmd_set_port_flag" }),
	"@set_portable_flags"  : ({ "cmd_set_port_flag" }),

	"@segment_map"         : ({ "cmd_segment_map" }),
	"@seg_map"             : ({ "cmd_segment_map" }),
	"@segmap"              : ({ "cmd_segment_map" }),

	"%od_report"           : ({ "cmd_od_report" }),
	"%list_dest"           : ({ "cmd_list_dest" }),
	"%full_rebuild"        : ({ "cmd_full_rebuild" }),

	"@log"                 : ({ "cmd_writelog" }),
	"%log"                 : ({ "cmd_writelog" }),
	"@writelog"            : ({ "cmd_writelog" }),
	"%writelog"            : ({ "cmd_writelog" }),
	"@write_log"           : ({ "cmd_writelog" }),
	"%write_log"           : ({ "cmd_writelog" }),
	"%log_subscribe"       : ({ "cmd_log_subscribe" }),

	"@help"                : ({ "cmd_help" }),
	"%help"                : ({ "cmd_help" }),

	"%people"              : ({ "cmd_people" }),
	"@people"              : ({ "cmd_people" }),
	"%who"                 : ({ "cmd_people" }),
	"@who"                 : ({ "cmd_people" }),

	"%status"              : ({ "cmd_status" }),
	"%get_config"          : ({ "cmd_get_config" }),
	"%print_config"        : ({ "cmd_get_config" }),

	"%shutdown"            : ({ "cmd_shutdown" }),
	"%reboot"              : ({ "cmd_reboot" }),
	"%swapout"             : ({ "cmd_swapout" }),
	"%statedump"           : ({ "cmd_statedump" }),
	"%datadump"            : ({ "cmd_datadump" }),
	"%save"                : ({ "cmd_datadump" }),
	"%safesave"            : ({ "cmd_safesave" }),

	"%code"                : ({ "cmd_code" }),
	"%history"             : ({ "cmd_history" }),
	"%clear"               : ({ "cmd_clear" }),
	"%compile"             : ({ "cmd_compile" }),
	"%clone"               : ({ "cmd_clone" }),
	"%destruct"            : ({ "cmd_destruct" }),

	"%access"              : ({ "cmd_access" }),
	"%grant"               : ({ "cmd_grant" }),
	"%ungrant"             : ({ "cmd_ungrant" }),
	"%quota"               : ({ "cmd_quota" }),
	"%rsrc"                : ({ "cmd_rsrc" }),

	"%ed"                  : ({ "cmd_ed" }),
	]),
	});

  /* Set up room & portable DTDs */
  if(room_dtd)
    room_dtd->clear();
  else
    room_dtd = clone_object(UNQ_DTD);

  if(port_dtd)
    port_dtd->clear();
  else
    port_dtd = clone_object(UNQ_DTD);

  dtd_file = read_entire_file(MAPD_ROOM_DTD);
  room_dtd->load(dtd_file);
  dtd_file = read_entire_file(PORTABLE_DTD);
  port_dtd->load(dtd_file);
}


mixed* parse_to_room(string room_file) {
  return UNQ_PARSER->unq_parse_with_dtd(room_file, room_dtd);
}

mixed* parse_to_portable(string room_file) {
  return UNQ_PARSER->unq_parse_with_dtd(room_file, port_dtd);
}

mixed* get_command_sets(object wiztool) {
  return command_sets;
}



/*
 * NAME:	message()
 * DESCRIPTION:	pass on a message to the user
 */
static void message(string str)
{
  if(user)
    user->message(str);
  else
    DRIVER->message("From wiztool.c: " + str);
}

/*
 * NAME:	command()
 * DESCRIPTION:	deal with input from user
 */
void command(string cmd, string str) {
  if(previous_object() == user) {
    call_limited("process_command", cmd, str);
  }
}

/*
 * NAME:	process_command()
 * DESCRIPTION:	process user input
 */
static void process_command(string cmd, string str)
{
  string arg;
  int    ctr;
  mixed* command_sets;

  if (query_editor(this_object())) {
    if (strlen(cmd) != 0 && cmd[0] == '!') {
      cmd = cmd[1 ..];
    } else {
      str = editor(str);
      if (str) {
	message(str);
      }
      return;
    }
  }

  if(!find_object(SYSTEM_WIZTOOL))
    compile_object(SYSTEM_WIZTOOL);

  command_sets = SYSTEM_WIZTOOL->get_command_sets(this_object());
  for(ctr = 0; ctr < sizeof(command_sets); ctr++) {
    if(command_sets[ctr][cmd]) {
      call_other(this_object(), command_sets[ctr][cmd][0], user, cmd,
		 str == "" ? nil : str);
      return;
    }
  }

  switch (cmd) {
  case "cd":
  case "pwd":
  case "ls":
  case "cp":
  case "mv":
  case "rm":
  case "mkdir":
  case "rmdir":

    call_other(this_object(), "cmd_" + cmd, user, cmd, str == "" ? nil : str);
    break;

  default:
    message("No command: " + cmd + "\n");
    break;
  }
}
