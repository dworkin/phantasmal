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


static string read_entire_file(string file) {
  string ret;

  ret = read_file(file);
  if (ret == nil) { return nil; }
  if(strlen(ret) > MAX_STRING_SIZE - 3) {
    error("File '" + file + "' is too large!");
  }

  return ret;
}


/*
 * NAME:	create()
 * DESCRIPTION:	initialize variables
 */
static void create(varargs int clone)
{

}

void destructed(varargs int clone) {

}


/********** Room Functions *****************************************/

/* List MUD rooms */
static void cmd_list_room(object user, string cmd, string str) {
  mixed*  rooms;
  int     ctr;
  string  tmp;

  if(str) {
    str = STRINGD->trim_whitespace(str);
    str = STRINGD->to_lower(str);
  }

  /* Just @list_rooms with no argument */
  if(str && (str == "all" || str == "world" || str == "mud")) {
    user->message("Rooms in MUD:\r\n");

    rooms = MAPD->rooms_in_zone(0);
    for(ctr = 0; ctr < sizeof(rooms); ctr++) {
      object room, phr;
 
      tmp = "";
      room = MAPD->get_room_by_num(rooms[ctr]);
      phr = room->get_glance();
      tmp += "  " + rooms[ctr] + "   ";
      tmp += phr->to_string(user);
      tmp += "\r\n";

      /* Output as each line finishes for debugging */
      user->message(tmp);
    }
    user->message("-----\r\n");

    return;
  }

  if(!str || str == "" || str == "zone") {
    object room;
    int    zone;
    int*   rooms;

    user->message("Rooms in zone:\r\n");

    room = user->get_location();
    zone = ZONED->get_zone_for_room(room);
    if(zone == -1)
      zone = 0;  /* Unzoned rooms */

    rooms = MAPD->rooms_in_zone(zone);
    for(ctr = 0; ctr < sizeof(rooms); ctr++) {
      object room, phr;

      tmp = "";
      room = MAPD->get_room_by_num(rooms[ctr]);
      phr = room->get_glance();
      tmp += "  " + rooms[ctr] + "   ";
      tmp += phr->to_string(user);
      tmp += "\r\n";

      /* Output as each line finishes for debugging */
      user->message(tmp);
    }

    user->message("-----\r\n");
    return;
  }

  user->message("Usage: " + cmd + "\r\n");
}


static void cmd_new_room(object user, string cmd, string str) {
  object room;
  int    roomnum;
  string segown;

  if(!str || STRINGD->is_whitespace(str)) {
    roomnum = -1;
  } else if(sscanf(str, "%*s %*s") == 2
	    || sscanf(str, "#%d", roomnum) != 1) {
    user->message("Usage: " + cmd + " [#room num]\r\n");
    return;
  }

  if(MAPD->get_room_by_num(roomnum)) {
    user->message("There is already a room with that number!\r\n");
    return;
  }

  segown = OBJNUMD->get_segment_owner(roomnum / 100);
  if(roomnum >= 0 && segown && segown != MAPD) {
    user->message("Room number " + roomnum
		  + " is in a segment reserved for non-rooms!\r\n");
    return;
  }

  room = clone_object(SIMPLE_ROOM);
  MAPD->add_room_number(room, roomnum);

  user->message("Added room #" + room->get_number() + ".\r\n");
}


static void cmd_delete_room(object user, string cmd, string str) {
  int    roomnum;
  object room;

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "%*s %*s") == 2 || !sscanf(str, "#%d", roomnum)) {
    user->message("Usage: " + cmd + " #<room num>\r\n");
    return;
  }

  room = MAPD->get_room_by_num(roomnum);
  if(!room) {
    user->message("No such room as #" + roomnum + ".\r\n");
    return;
  }

  EXITD->clear_all_exits(room);

  if(room->get_location()) {
    room->get_location()->remove_from_container(room);
  }

  destruct_object(room);
}


static void cmd_save_rooms(object user, string cmd, string str) {
  string unq_str, argstr;
  mixed* rooms, *args;
  object room;
  int    ctr;

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + " <file to write>\r\n");
    user->message("   or  " + cmd
		  + " <file to write> #<num> #<num> #<num>...\r\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);
  remove_file(str + ".old");
  rename_file(str, str + ".old");  /* Try to remove & rename, just in case */

  if(sizeof(get_dir(str)[0])) {
    user->message("Couldn't make space for file -- can't overwrite!\r\n");
    return;
  }

  if(sscanf(str, "%*s %*s") != 2) {
    rooms = MAPD->rooms_in_zone(0) - ({ 0 });
  } else {
    int roomnum;

    rooms = ({ });
    sscanf(str, "%s %s", str, argstr);
    args = explode(argstr, " ");
    for(ctr = 0; ctr < sizeof(args); ctr++) {
      if(sscanf(args[ctr], "#%d", roomnum)) {
	rooms += ({ roomnum });
      } else {
	user->message("'" + args[ctr] + "' is not a valid room number.\r\n");
	return;
      }
    }
  }

  if(!rooms || !sizeof(rooms)) {
    user->message("No rooms to save!\r\n");
    return;
  }

  user->message("Saving rooms: ");
  for(ctr = 0; ctr < sizeof(rooms); ctr++) {
    room = MAPD->get_room_by_num(rooms[ctr]);

    unq_str = room->to_unq_text();

    if(!write_file(str, unq_str))
      error("Couldn't write rooms to file " + str + "!");
    user->message(".");
  }

  user->message("\r\nDone!\r\n");
}

static void cmd_load_rooms(object user, string cmd, string str) {
  string room_file, argstr;
  mixed* unq_data, *tmp, *args, *rooms;
  int    iter, roomnum;

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Currently only those with full administrative access "
		  + "may load rooms.\r\n");
    return;
  }

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + " <file to load>\r\n");
    return;
  }

  /* If it looks like the admin specified rooms, parse them */
  if(sscanf(str, "%s %s", str, argstr) == 2) {
    rooms = ({ });
    args = explode(argstr, " ");
    for(iter = 0; iter < sizeof(args); iter++) {
      if(sscanf(args[iter], "#%d", roomnum)) {
	rooms += ({ roomnum });
      } else {
	user->message("'" + args[iter]
		      + "' doesn't look like a room number!\r\n");
	return;
      }
    }
  }

  /* Check validity of file */
  str = STRINGD->trim_whitespace(str);
  if(!sizeof(get_dir(str)[0])) {
    user->message("Can't find file: " + str + "\r\n");
    return;
  }
  room_file = read_file(str);
  if(!room_file || !strlen(room_file)) {
    user->message("Error reading room file, or file is empty.\r\n");
    return;
  }
  if(strlen(room_file) > MAX_STRING_SIZE - 3) {
    user->message("Room file is too big!  "
		  + "See Angelbob to increase current limit.");
    return;
  }

  tmp = UNQ_PARSER->basic_unq_parse(room_file);
  if(!tmp) {
    user->message("Cannot parse text as UNQ adding UNQ rooms!\r\n");
    return;
  }
  tmp = SYSTEM_WIZTOOL->parse_to_room(room_file);
  if(!tmp) {
    user->message("Cannot parse UNQ as rooms adding UNQ rooms!\r\n");
    return;
  }

  if(rooms) {
    unq_data = ({ });
    for(iter = 0; iter < sizeof(tmp); iter += 2) {
      if(tmp[iter + 1][1][0] == "number") {
	roomnum = tmp[iter + 1][1][1];
	if( sizeof(({ roomnum }) & rooms) ) {
	  unq_data += tmp[iter..iter + 1];
	  rooms -= ({ roomnum });
	}
      }
    }
  } else {
    unq_data = tmp[..];
  }

  if(rooms && sizeof(rooms)) {
    string tmp;

    tmp = "No match loading room numbers: ";
    for(iter = 0; iter < sizeof(rooms); iter++) {
      tmp += "#" + rooms[iter] + " ";
    }
    tmp += "\r\n";
    user->message(tmp);

    if(sizeof(unq_data)) {
      user->message("Attempting remaining rooms:\r\n\r\n");
    } else {
      user->message("No specified rooms found, ignoring.\r\n");
      return;
    }
  }

  user->message("Registering rooms...\r\n");
  MAPD->add_dtd_unq_rooms(unq_data, str);
  user->message("Resolving exits...\r\n");
  EXITD->add_deferred_exits();
  user->message("Done.\r\n");
}



/******  Exit Functions ****************************************/

static void cmd_goto_room(object user, string cmd, string str) {
  int    roomnum;
  object room;
  object mob;

  if(sscanf(str, "#%d", roomnum)) {
    room = MAPD->get_room_by_num(roomnum);
    if(!room) {
      user->message("Can't locate room #" + roomnum + "\r\n");
      return;
    }
  } else {
    user->message("Usage: " + cmd + " #<location num>\r\n");
    return;
  }

  mob = user->get_body()->get_mobile();

  user->message("You teleport to " + room->get_brief()->to_string(user)
		+ ".\r\n");
  mob->teleport(room, 1);
  user->show_room_to_player(user->get_location());
}

static void cmd_new_exit(object user, string cmd, string str) {
  int    roomnum, dir;
  object room;
  string dirname;

  if(str && sscanf(str, "%s #%d", dirname, roomnum) == 2) {
    room = MAPD->get_room_by_num(roomnum);
    if(!room) {
      user->message("Can't locate room #" + roomnum + "\r\n");
      return;
    }
    dir = EXITD->direction_by_string(dirname);
    if(dir == -1) {
      user->message("Don't recognize " + dirname + " as direction.\r\n");
      return;
    }
  } else {
    user->message("Usage: " + cmd + " <direction> #<room number>\r\n");
    return;
  }

  if(!user->get_location()) {
    user->message("You aren't standing anywhere so you can't make an exit!\r\n");
    return;
  }
  if(user->get_location()->get_exit(dir)) {
    user->message("There already appears to be an exit in that direction.\r\n");
    return;
  }

  user->message("You begin creating an exit to '"
		+ room->get_brief()->to_string(user) + "'.\r\n");
  EXITD->add_simple_exit_between(user->get_location(), room, dir, -1, -1);
}

static void cmd_clear_exits(object user, string cmd, string str) {
  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  if(!user->get_location()) {
    user->message("You aren't anywhere, so you can't " + cmd + "!\r\n");
    return;
  }
  EXITD->clear_all_exits(user->get_location());
}

static void cmd_remove_exit(object user, string cmd, string str) {
  int    dir;
  object exit;

  if(!str || STRINGD->is_whitespace(str) || sscanf(str, "%*s %*s") == 2) {
    user->message("Usage: " + cmd + " <direction>\r\n");
    return;
  }

  str = STRINGD->trim_whitespace(str);
  dir = EXITD->direction_by_string(str);
  if(dir == -1) {
    user->message("'" + str + "' is not a direction.\r\n");
    return;
  }

  if(!user->get_location()) {
    user->message("You aren't anywhere, so you can't " + cmd + "!\r\n");
    return;
  }
  exit = user->get_location()->get_exit(dir);
  if(!exit) {
    user->message("No exit found going " + str + ".\r\n");
    return;
  }

  EXITD->remove_exit(user->get_location(), exit);
}

static void cmd_list_exit(object user, string cmd, string str) {
  int   *tmp, *segs, *exits;
  int    ctr;
  string tmpstr;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  /* Ignore cmd, str for the moment */
  segs = EXITD->get_exit_segments();
  if(!segs) {
    user->message("Can't get segments for exits!\r\n");
    return;
  }

  tmp = ({ });
  exits = ({ });
  for(ctr = 0; ctr < sizeof(segs); ctr++) {
    tmp = EXITD->exits_in_segment(segs[ctr]);
    if(tmp && sizeof(tmp))
      exits += tmp;
  }

  user->message("Exits:\r\n");
  tmpstr = "";
  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    object exit, dest, phr;
    int    dir;

    exit = EXITD->get_exit_by_num(exits[ctr]);
    if(!exit)
      error("Can't find exit #" + exits[ctr]);

    dest = exit->get_destination();
    tmpstr += "Exit #" + exit->get_number();

    if(exit->get_brief())
      tmpstr += " (" + exit->get_brief()->to_string(user) + ") ";
    else
      tmpstr += " (no desc) ";

    dir = exit->get_direction();
    if(dir != -1) {
      phr = EXITD->get_name_for_dir(dir);
      if(!phr)
	tmpstr += "undef direction " + dir;
      else
	tmpstr += phr->to_string(user);
    } else
      tmpstr += "no direction";

    /* Show "to" location as number or commentary */
    tmpstr += " to #";
    if(!dest)
      tmpstr += "<none>";
    else if (dest->get_number() == -1)
      tmpstr += "<unregistered room>";
    else
      tmpstr += dest->get_number();
    tmpstr += "\r\n";
  }

  user->message(tmpstr);

  ctr = EXITD->num_deferred_exits();
  if(ctr)
    user->message("Deferred exits: " + ctr + ".\r\n");

}

static void cmd_add_deferred_exits(object user, string cmd, string str) {
  int num;

  num = EXITD->num_deferred_exits();
  user->message("Deferred exits: " + num + ".\r\n");

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Currently only those with full administrative access "
		  + "may resolve deferred exits.\r\n");
    return;
  }

  EXITD->add_deferred_exits();

  num = EXITD->num_deferred_exits();
  user->message("Deferred exits: " + num + ".\r\n");
}

static void cmd_check_deferred_exits(object user, string cmd, string str) {
  int num;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(str && str != "") {
    user->message("Usage: " + cmd + "\r\n");
  }

  num = EXITD->num_deferred_exits();

  user->message("Deferred exits: " + num + ".\r\n");
}


static void cmd_add_detail(object user, string cmd, string str) {
  int objnum, detailnum;

  if(str)
    str = STRINGD->trim_whitespace(str);

  detailnum = -1;
  if(sscanf(str, "#%*d #%*d %*s") == 3
     || (sscanf(str, "#%d #%d", objnum, detailnum) != 2
	 && sscanf(str, "#%d", objnum) != 1)) {
    user->message("Usage: " + cmd + " #<objnum> [#<detailnum>]\r\n");
    return;
  }

  
}
