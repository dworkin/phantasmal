#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>
#include <trace.h>
#include <type.h>
#include <status.h>
#include <limits.h>
#include <log.h>

inherit auto AUTO;
inherit wiz LIB_WIZTOOL;
inherit access API_ACCESS;

inherit roomwiz "/usr/System/lib/room_wiztool";
inherit objwiz "/usr/System/lib/obj_wiztool";

private string owner;		/* owner of this object */
private string directory;	/* current directory */

private object port_dtd;        /* DTD for portable def'n */


#define SPACE16 "                "

/*
 * NAME:	ralign()
 * DESCRIPTION:	return a number as a right-aligned string
 */
private string ralign(mixed num, int width)
{
    string str;

    str = SPACE16 + (string) num;
    return str[strlen(str) - width ..];
}


/*
 * NAME:	create()
 * DESCRIPTION:	initialize variables
 */
static void create(varargs int clone)
{
  wiz::create(200);
  access::create();
  roomwiz::create(clone);
  objwiz::create(clone);

  if(clone) {
    owner = query_owner();
    directory = USR + "/" + owner;
  } else {
    if(!find_object(US_OBJ_DESC))
      auto::compile_object(US_OBJ_DESC);
    if(!find_object(US_ENTER_DATA))
      auto::compile_object(US_ENTER_DATA);
    if(!find_object(UNQ_DTD))
      auto::compile_object(UNQ_DTD);
    if(!find_object(SIMPLE_PORTABLE))
      auto::compile_object(SIMPLE_PORTABLE);
    if(!find_object(SIMPLE_ROOM))
      auto::compile_object(SIMPLE_ROOM);
    if(!find_object(SIMPLE_DETAIL))
      auto::compile_object(SIMPLE_DETAIL);
  }
}

void destructed(int clone) {
  roomwiz::destructed(clone);
  objwiz::destructed(clone);
}


static void cmd_shutdown(object user, string cmd, string str)
{
  find_object(INITD)->prepare_shutdown();
  /* wiz::cmd_shutdown(user, cmd, str); */
}

static void cmd_reboot(object user, string cmd, string str) {
  /* DRIVER will do this for us, so we don't need to */
  /* find_object(INITD)->prepare_reboot(); */
  wiz::cmd_reboot(user, cmd, str);
}

static void cmd_datadump(object user, string cmd, string str) {
  find_object(INITD)->save_mud_data(user, ROOM_FILE, PORT_FILE, ZONE_FILE,
				    nil);
  user->message("Data save commenced.\n");
}

static void cmd_safesave(object user, string cmd, string str) {
  find_object(INITD)->save_mud_data(user, SAFE_ROOM_FILE, SAFE_PORT_FILE,
				    SAFE_ZONE_FILE, nil);
  user->message("Safe data save commenced.\n");
}

static void cmd_compile(object user, string cmd, string str)
{
  string objname;

  user->message("Compiling '" + str + "'.\r\n");

  if(!sscanf(str, "$%*d") && sscanf(str, "%s.c", objname)) {
    mixed* status;

    status = OBJECTD->od_status(objname);
    if(status) {
      /* Check to see if there are children and most recent issue is
	 destroyed... */
      if(status[3] && sizeof(status[3]) && !status[6]) {
	user->message("Can't recompile -- library issue has children!\r\n");
	return;
      }
    }
  }

  catch {
    wiz::cmd_compile(user, cmd, str);
  } : {
    if(ERRORD->last_compile_errors()) {
      user->message("===Compile errors:\r\n" + ERRORD->last_compile_errors());
      user->message("---\r\n");
    }

    if(ERRORD->last_runtime_error()) {
      if(sscanf(ERRORD->last_runtime_error(),
		"%*sFailed to compile%*s") == 2) {
	return;
      }

      user->message("===Runtime error: '" + ERRORD->last_runtime_error()
		    + "'.\r\n");
      user->message("---\r\n");
    }

    if(ERRORD->last_stack_trace()) {
      user->message("===Stack trace: '" + ERRORD->last_stack_trace()
		    + "'.\r\n");
      user->message("---\r\n");
    }

    return;
  }

  user->message("Done.\r\n");
}


static void cmd_destruct(object user, string cmd, string str)
{
  user->message("Destructing '" + str + "'.\r\n");

  catch {
    wiz::cmd_destruct(user, cmd, str);
  } : {
    if(ERRORD->last_runtime_error()) {
      user->message("===Runtime error: '" + ERRORD->last_runtime_error()
		    + "'.\r\n");
      user->message("---\r\n");
    }

    if(ERRORD->last_stack_trace()) {
      user->message("===Stack trace: '" + ERRORD->last_stack_trace()
		    + "'.\r\n");
      user->message("---\r\n");
    }

    return;
  }

  user->message("Done.\r\n");
}


static void cmd_get_config(object user, string cmd, string str) {
  object configd;

  configd = find_object(CONFIGD);
  if(!configd) {
    user->message("Can't find the CONFIGD!\n\r");
    return;
  }

  user->message("Configuration parameters:\n\r");
  user->message("   Start Room:     " + configd->get_start_room() + "\n\r");
  user->message("  Meat Locker:     " + configd->get_meat_locker() + "\n\r");
  user->message("-----\n\r");
}


static void cmd_people(object user, string cmd, string str)
{
  object *users, usr;
  string *owners, name;
  int i, sz;

  if (str) {
    message("Usage: " + cmd + "\r\n");
    return;
  }

  str = "";
  users = users();
  owners = query_owners();
  for (i = 0, sz = sizeof(users); i < sz; i++) {
    usr = users[i];
    name = usr->query_name();
    str += (query_ip_number(usr->query_conn()) + SPACE16)[.. 15] +
      (usr->get_idle_time() + " seconds idle" + SPACE16)[..18] +
      ((sizeof(owners & ({ name })) == 0) ? " " : "*") +
      name + "\r\n";
  }
  message(str);
}

static void cmd_writelog(object user, string cmd, string str)
{
  if(str) {
    LOGD->write_syslog(str, LOG_ERR_FATAL);
  } else {
    user->message("Usage: " + cmd + " <string to log>\r\n");
  }
}

static void cmd_log_subscribe(object user, string cmd, string str) {
  string chan, levname;
  int    lev;

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user
      ->message("Can't set logfile subscriptions without full admin access!");
    return;
  }

  if(str && sscanf(str, "%s %d", chan, lev) == 2) {
    LOGD->set_channel_sub(chan, lev);
    user->message("Setting channel sub for '" + chan + "' to "
		  + lev + "\r\n");
    return;
  } else if (str && sscanf(str, "%s %s", chan, levname) == 2
	     && LOGD->get_level_by_name(levname)) {
    int level;

    level = LOGD->get_level_by_name(levname);
    LOGD->set_channel_sub(chan, level);
    user->message("Setting channel sub for '" + chan + "' to "
		  + level + "\r\n");
    return;
  } else if (str && sscanf(str, "%s", chan)) {
    lev = LOGD->channel_sub(chan);
    if(lev == -1) {
      user->message("No subscription to channel '" + chan + "'\r\n");
    } else {
      user->message("Sub to channel '" + chan + "' is " + lev + "\r\n");
    }
    return;
  } else {
    user->message("Usage: %log_subscribe <channel> <level>\r\n");
  }
}

static void cmd_list_dest(object user, string cmd, string str)
{
  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  if(!find_object(OBJECTD))
    auto::compile_object(OBJECTD);

  user->message(OBJECTD->destroyed_obj_list());
}

static void cmd_od_report(object user, string cmd, string str)
{
  int    i, hmax;
  mixed  obj;
  string report;

  if(!find_object(OBJECTD))
    auto::compile_object(OBJECTD);

  hmax = sizeof(::query_history());

  i = -1;
  if(!str || (sscanf(str, "$%d%s", i, str) == 2 &&
	      (i < 0 || i >= hmax || str != ""))) {
    message("Usage: " + cmd + " <obj> | $<ident>\r\n");
    return;
  }

  if (i >= 0) {
    obj = fetch(i);
    if(typeof(obj) != T_OBJECT) {
      message("Not an object.\r\n");
      return;
    }
  } else if (sscanf(str, "$%s", str)) {
    obj = ::ident(str);
    if (!obj) {
      message("Unknown: $ident.\r\n");
      return;
    }
  } else if (sscanf(str, "#%*d")) {
    obj = str;
  } else if (sscanf(str, "%*d")) {
    obj = str;
  } else {
    obj = DRIVER->normalize_path(str, directory, owner);
  }

  str = catch(report = OBJECTD->report_on_object(obj));
  if(str) {
    str += "\r\n";
  } else if (!report) {
    str = "Nil report from Object Manager!\r\n";
  } else {
    str = report;
  }

  message(str);
}


static void cmd_full_rebuild(object user, string cmd, string str) {
  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Currently only those with full administrative access "
		  + "may do a full rebuild.\r\n");
    return;
  }

  user->message("Recompiling auto object...\r\n");

  catch {
    OBJECTD->recompile_auto_object(user);
  } : {
    if(ERRORD->last_compile_errors()) {
      user->message("===Compile errors:\r\n" + ERRORD->last_compile_errors());
      user->message("---\r\n");
    }

    if(ERRORD->last_runtime_error()) {
      if(sscanf(ERRORD->last_runtime_error(),
		"%*sFailed to compile%*s") == 2) {
	return;
      }

      user->message("===Runtime error: '" + ERRORD->last_runtime_error()
		    + "'.\r\n");
      user->message("---\r\n");
    }

    if(ERRORD->last_stack_trace()) {
      user->message("===Stack trace: '" + ERRORD->last_stack_trace()
		    + "'.\r\n");
      user->message("---\r\n");
    }

    return;
  }

  user->message("Done.\r\n");
}


static void cmd_help(object user, string cmd, string str) {
  mixed *hlp;
  int index;

  if (str && sscanf(str, "%d %s", index, str) == 2) {
    if(index < 1) {
      user->message("Usage: " + cmd + " <word>\r\n");
      user->message("   or  " + cmd + " <num> <word>\r\n");
      return;
    }
    index = index - 1;  /* User sees 1-indexed, we see 0-indexed */
  } else if (!str || STRINGD->is_whitespace(str)) {
    str = "main_admin";
    index = 0;
  } else {
    index = 0;
  }

  hlp = HELPD->query_exact_with_keywords(str, user, ({ "admin" }));
  if(hlp) {
    if((sizeof(hlp) <= index) || (sizeof(hlp) <= 0)
       || (index < 0)) {
      user->message("Only " + sizeof(hlp) + " help files on \""
		    + str + "\".\r\n");
    } else {
      if(sizeof(hlp) > 1) {
	user->message("Help on " + str + ":    [" + sizeof(hlp)
		      + " entries]\r\n");
      }
      user->message(hlp[index][1]->to_string(user));
      user->message("\r\n");
    }
    return;
  }

  user->message("No help on \"" + str + "\".\r\n");
}



static void cmd_new_portable(object user, string cmd, string str) {
  object port;
  int    portnum;
  string segown;

  if(!str || STRINGD->is_whitespace(str)) {
    portnum = -1;
  } else if(sscanf(str, "%*s %*s") == 2
	    || sscanf(str, "#%d", portnum) != 1) {
    user->message("Usage: " + cmd + " [#port num]\r\n");
    return;
  }

  if(PORTABLED->get_port_by_num(portnum)) {
    user->message("There is already a portable with that number!\r\n");
    return;
  }

  segown = OBJNUMD->get_segment_owner(portnum / 100);
  if(portnum >= 0 && segown && segown != PORTABLED) {
    user->message("That number is in a segment reserved for non-portables!\r\n");
    return;
  }

  port = clone_object(SIMPLE_PORTABLE);
  if(!port)
    error("Can't clone simple portable!");
  PORTABLED->add_portable_number(port, portnum);
  if(!PORTABLED->get_portable_by_num(port->get_number())) {
    LOGD->write_syslog("Urgh -- error in cmd_new_portable!", LOG_ERR);
  }

  if(user->get_location()) {
    user->get_location()->add_to_container(port);
  }

  user->message("Added portable #" + port->get_number() + ".\r\n");
}

static void cmd_delete_portable(object user, string cmd, string str) {
  object port;
  int    portnum;

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "%*s %*s") == 2 || !sscanf(str, "#%d", portnum)) {
    user->message("Usage: " + cmd + " #<portable num>\r\n");
    return;
  }

  port = PORTABLED->get_portable_by_num(portnum);
  if(!port) {
    user->message("No portable by that number (#" + portnum + ").\r\n");
    return;
  }

  /* TODO: do anything special for contents?  Default destructor for
     portables will do pretty well here... */

  if(port->get_location()) {
    port->get_location()->remove_from_container(port);
  }

  destruct_object(port);
}

static void cmd_list_portables(object user, string cmd, string str) {
  int*    portables;
  int*    port_seg;
  int     ctr;
  string  tmp;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  user->message("Portables:\r\n");

  portables = ({ });
  port_seg = PORTABLED->get_portable_segments();
  for(ctr = 0; ctr < sizeof(port_seg); ctr++) {
    int* tmp;

    tmp = PORTABLED->portables_in_segment(port_seg[ctr]);
    if(tmp) {
      portables += tmp;
    }
  }

  tmp = "";
  for(ctr = 0; ctr < sizeof(portables); ctr++) {
    object port, phr;

    port = PORTABLED->get_portable_by_num(portables[ctr]);
    phr = port->get_glance();
    tmp += "  " + portables[ctr] + "   ";
    tmp += phr->to_string(user);
    tmp += "\r\n";

    /* Output as each line finishes for debugging */
    user->message(tmp);
    tmp = "";
  }
  user->message("-----\r\n");

}


static void cmd_segment_map(object user, string cmd, string str) {
  int hs, ctr;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  user->message("Segments:\r\n");
  hs = OBJNUMD->get_highest_segment();
  for(ctr = 0; ctr <= hs; ctr++) {
    user->message((ctr + SPACE16)[..6]
		  + (OBJNUMD->get_segment_owner(ctr) + SPACE16)[..30]
		  + OBJNUMD->get_segment_zone(ctr)
		  + "\r\n");
  }
  user->message("--------\r\n");
}


static void cmd_set_segment_zone(object user, string cmd, string str) {
  int segnum, zonenum;

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "#%d #%d", segnum, zonenum) != 2) {
    user->message("Usage: " + cmd + " #<segnum> #<zonenum>\r\n");
    return;
  }

  if(!OBJNUMD->get_segment_owner(segnum)) {
    user->message("Can't find that segment number!\r\n");
    return;
  }
  OBJNUMD->set_segment_zone(segnum, zonenum);

  user->message("Set segment #" + segnum + " to be in zone #" + zonenum
		+ ".\r\n");
}


static void cmd_zone_map(object user, string cmd, string str) {
  int ctr, num_zones;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  num_zones = ZONED->num_zones();

  user->message("Zones:\r\n");
  for(ctr = 0; ctr < num_zones; ctr++) {
    user->message(ralign(ctr + "", 3) + ": " + ZONED->get_name_for_zone(ctr)
		  + "\r\n");
    /* user->message("  1:   Miskatonic University\r\n"); */
  }
  user->message("-----\r\n");
}


static void cmd_save_portables(object user, string cmd, string str) {
  string unq_str, argstr;
  mixed* ports, *args;
  object port;
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
    int* segments, *tmp;

    ports = ({ });
    segments = PORTABLED->get_portable_segments();
    for(ctr = 0; ctr < sizeof(segments); ctr++) {
      tmp = PORTABLED->portables_in_segment(segments[ctr]);
      if(tmp)
	ports += tmp;
    }
  } else {
    int portnum;

    ports = ({ });
    sscanf(str, "%s %s", str, argstr);
    args = explode(argstr, " ");
    for(ctr = 0; ctr < sizeof(args); ctr++) {
      if(sscanf(args[ctr], "#%d", portnum)) {
	ports += ({ portnum });
      } else {
	user->message("'" + args[ctr] + "' is not a valid portable number.\r\n");
	return;
      }
    }
  }

  if(!ports || !sizeof(ports)) {
    user->message("No portables to save!\r\n");
    return;
  }

  user->message("Saving portables: ");
  for(ctr = 0; ctr < sizeof(ports); ctr++) {
    port = PORTABLED->get_portable_by_num(ports[ctr]);

    if(!port) {
      user->message("\r\nCan't find portable #" + ports[ctr] + ".\r\n");
      continue;
    }

    unq_str = port->to_unq_text();

    if(!write_file(str, unq_str))
      error("Couldn't write portables to file " + str + "!");
    user->message(".");
  }

  user->message("\r\nDone!\r\n");
}


static void cmd_load_portables(object user, string cmd, string str) {
  string port_file, argstr;
  mixed* unq_data, *tmp, *args, *ports;
  int    iter, portnum;
  object port;

  if(!access(user->query_name(), "/", FULL_ACCESS)) {
    user->message("Currently only those with full administrative access "
		  + "may load portables.\r\n");
    return;
  }

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + " <file to load>\r\n");
    return;
  }

  /* If it looks like the admin specified portables, parse them */
  if(sscanf(str, "%s %s", str, argstr) == 2) {
    ports = ({ });
    args = explode(argstr, " ");
    for(iter = 0; iter < sizeof(args); iter++) {
      if(sscanf(args[iter], "#%d", portnum)) {
	ports += ({ portnum });
      } else {
	user->message("'" + args[iter]
		      + "' doesn't look like a portable number!\r\n");
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
  port_file = read_file(str);
  if(!port_file || !strlen(port_file)) {
    user->message("Error reading portable file, or file is empty.\r\n");
    return;
  }
  if(strlen(port_file) > MAX_STRING_SIZE - 3) {
    user->message("Portable file is too big!  "
		  + "See Angelbob to increase current limit.");
    return;
  }

  tmp = UNQ_PARSER->basic_unq_parse(port_file);
  if(!tmp) {
    user->message("Cannot parse text as UNQ while adding portables!");
    return;
  }
  tmp = SYSTEM_WIZTOOL->parse_to_portable(port_file);
  if(!tmp) {
    user->message("Cannot parse UNQ as portable while adding portables!\r\n");
    return;
  }

  if(ports) {
    unq_data = ({ });
    for(iter = 0; iter < sizeof(tmp); iter += 2) {
      if(tmp[iter + 1][1][0] == "number") {
	portnum = tmp[iter + 1][1][1];
	if( sizeof(({ portnum }) & ports) ) {
	  unq_data += tmp[iter..iter + 1];
	  ports -= ({ portnum });
	}
      }
    }
  } else {
    unq_data = tmp[..];
  }

  if(ports && sizeof(ports)) {
    string tmp;

    tmp = "No match loading portable numbers: ";
    for(iter = 0; iter < sizeof(ports); iter++) {
      tmp += "#" + ports[iter] + " ";
    }
    tmp += "\r\n";
    user->message(tmp);

    if(sizeof(unq_data)) {
      user->message("Attempting remaining portables:\r\n\r\n");
    } else {
      user->message("No specified portables found, ignoring.\r\n");
      return;
    }
  }

  user->message("Registering portables...\r\n");
  PORTABLED->add_dtd_unq_portables(unq_data, user->get_location(), str);
  user->message("Done.\r\n");
}

static void cmd_set_port_flag(object user, string cmd, string str) {
  string flag, vstring;
  int    objnum, val;
  object port;

  val = 1;

  if(sscanf(str, "#%d %s %d", objnum, flag, val) != 3
     && sscanf(str, "#%d %s %s", objnum, flag, vstring) != 3
     && sscanf(str, "#%d %s", objnum, flag) != 2) {
    user->message("Usage: " + cmd + " #<portnum> <flagname> [<val>]\r\n");
    return;
  }

  if(vstring) {
    if(vstring == "on"
       || vstring == "yes"
       || vstring == "true") {
      val = 1;
    } else if(vstring == "off"
	      || vstring == "no"
	      || vstring == "false") {
      val = 0;
    } else {
      user->message("Don't recognize '" + vstring + "' as true or false.\r\n");
      return;
    }
  }

  port = PORTABLED->get_portable_by_num(objnum);
  if(!port) {
    user->message("Don't recognize object #" + objnum + " as a portable.\r\n");
    return;
  }

  if(flag == "con" || flag == "cont" || flag == "container") {
    port->set_container(val);
  } else if (flag == "open" || flag == "op") {
    port->set_open(val);
  } else if (flag == "cl" || flag == "close" || flag == "closed") {
    port->set_open(!val);
  } else if (flag == "nodesc" || flag == "undesc" || flag == "no_desc"
	     || flag == "descless") {
    port->set_no_desc(val);
  } else {
    user->message("Don't recognize '" + flag + "' as a portable flag.\r\n");
    return;
  }

  user->message("Done.\r\n");
}
