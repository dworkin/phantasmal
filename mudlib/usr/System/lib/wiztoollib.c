#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <phantasmal/log.h>

#include <trace.h>
#include <type.h>
#include <status.h>
#include <limits.h>

inherit auto AUTO;
inherit wiz LIB_WIZTOOL;
inherit access API_ACCESS;

inherit roomwiz SYSTEM_ROOMWIZTOOLLIB;
inherit objwiz  SYSTEM_OBJWIZTOOLLIB;

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
    if(!find_object(SIMPLE_ROOM))
      auto::compile_object(SIMPLE_ROOM);
  }
}

void destructed(int clone) {
  if(SYSTEM()) {
    roomwiz::destructed(clone);
    objwiz::destructed(clone);
  }
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
  find_object(INITD)->save_mud_data(user, ROOM_DIR, MOB_FILE, ZONE_FILE,
				    nil);
  user->message("Data save commenced.\n");
}

static void cmd_safesave(object user, string cmd, string str) {
  find_object(INITD)->save_mud_data(user, SAFE_ROOM_DIR, SAFE_MOB_FILE,
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

  if(!sscanf(str, "$%*d") && !sscanf(str, "%*s.c")) {
    if(!read_file(str, 0, 1) && read_file(str + ".c", 0, 1)) {
      user->message("(compiling " + str + ".c)\r\n");
      str += ".c";
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


static void cmd_list_mobiles(object user, string cmd, string str) {
  int*   mobiles;
  int    ctr;
  object mob, phr;
  string tmp;

  if(str && !STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + "\r\n");
    return;
  }

  mobiles = MOBILED->all_mobiles();

  for(ctr = 0; ctr < sizeof(mobiles); ctr++) {
    mob = MOBILED->get_mobile_by_num(mobiles[ctr]);
    tmp = ralign("" + mobiles[ctr], 8);
    tmp += "   ";

    tmp += ralign(mob->get_type(), 8);
    tmp += "     ";

    if(mob->get_body()) {
      phr = mob->get_body()->get_glance();
      tmp += phr->to_string(user);
    } else {
      tmp += "<bodiless mob>";
    }
    tmp += "\r\n";
    user->message(tmp);
  }
  user->message("-----\r\n");
}


static void cmd_delete_mobile(object user, string cmd, string str) {
  int    mobnum;
  object mob, body, location;

  if(!str || STRINGD->is_whitespace(str)
     || sscanf(str, "%*s %*s") == 2
     || sscanf(str, "#%d", mobnum) != 1) {
    user->message("Usage: " + cmd + " #<mobile number>\r\n");
    return;
  }

  mob = MOBILED->get_mobile_by_num(mobnum);
  if(!mob) {
    user->message("No mobile #" + mobnum
		  + " is registered with MOBILED.  Failed.\r\n");
    return;
  }

  if(mob->get_user()) {
    user->message("Mobile is still hooked up to a network connection."
		  + "  Failed.\r\n");
    return;
  }

  /* Need to remove mobile from any room lists it currently occupies. */
  MOBILED->remove_mobile(mob);

  user->message("Mobile #" + mobnum + " successfully destructed.\r\n");
}


static void cmd_delete_obj(object user, string cmd, string str) {
  object *objs;
  int     obj_num;

  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + " #<object number>\r\n");
    user->message("   or  " + cmd + " object description\r\n");
    return;
  }

  if(sscanf(str, "#%*d %*s") != 2
     && sscanf(str, "%*s #%*d") != 2
     && sscanf(str, "#%d", obj_num) == 1) {
    /* Delete by object number */

  } else {
    /* Delete by object name */

    str = STRINGD->trim_whitespace(str);
    if(user->get_location()) {
      objs = user->get_location()->find_contained_objects(user, str);
      if(!objs)
	objs = user->get_body()->find_contained_objects(user, str);
      if(!objs || !sizeof(objs)) {
	user->message("There's nothing matching '" + str + "'.\r\n");
	return;
      }
      if(sizeof(objs) > 1) {
	user->message("There are multiple things matching '" + str + "'.\r\n");
	user->message("Specify just one.\r\n");
	return;
      }
      obj_num = objs[0]->get_number();
    } else {
      user->message("You're nowhere.  You can't delete things there.\r\n");
      return;
    }
  }

  if(MOBILED->get_mobile_by_num(obj_num)) {
    /* Do a mobile delete */
    cmd_delete_mobile(user, "@delete_mobile", "#" + obj_num);
  } else if(MAPD->get_room_by_num(obj_num)) {
    /* Do a room delete */
    cmd_delete_room(user, "@delete_room", "#" + obj_num);
  } else if(EXITD->get_exit_by_num(obj_num)) {
    object exit;

    user->message("Removing exit...\r\n");
    /* Do an exit delete */
    exit = EXITD->get_exit_by_num(obj_num);
    EXITD->clear_exit(exit);

    user->message("Done.\r\n");
  } else {
    user->message("That's not a portable, a room, a mobile or an exit.\r\n");
    user->message("Either it doesn't exist, or @delete can't delete it.\r\n");
    return;
  }
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
    user->message((OBJNUMD->get_segment_owner(ctr) != nil) ?
                ((ctr + SPACE16)[..6]
                + (OBJNUMD->get_segment_owner(ctr) + SPACE16)[..30]
                + OBJNUMD->get_segment_zone(ctr)
                + "\r\n") : "");
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
    user->message("Can't find segment #" + segnum + ".  Try @segmap.\r\n");
    return;
  }
  if(zonenum >= ZONED->num_zones()) {
    user->message("Can't find zone #" + zonenum + ".  Try @zonemap.\r\n");
    return;
  }
  OBJNUMD->set_segment_zone(segnum, zonenum);

  user->message("Set segment #" + segnum + " (object #" + (segnum * 100)
		+ "-#" + (segnum * 100 + 99) + ") to be in zone #" + zonenum
		+ " (" + ZONED->get_name_for_zone(zonenum) + ").\r\n");
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
  }
  user->message("-----\r\n");
}

static void cmd_new_zone(object user, string cmd, string str) {
  int ctr, new_zonenum;
  if(!str || STRINGD->is_whitespace(str)) {
    user->message("Usage: " + cmd + " <zone name>\r\n");
    return;
  }

  new_zonenum = ZONED->add_new_zone( str );

  user->message("Added zone #"+new_zonenum+"\r\n");
}

static void cmd_new_mobile(object user, string cmd, string str) {
  int    mobnum, bodynum;
  string mobtype, segown;
  object mobile, body;

  mobnum = -1;
  if(!str || sscanf(str, "%*s %*s %*s %*s") == 4
     || ((sscanf(str, "#%d %s", bodynum, mobtype) != 2)
	 && (sscanf(str, "#%d #%d %s", mobnum, bodynum, mobtype) != 3))) {
    user->message("Usage: " + cmd
		  + " #<new mob num> #<body num> <mobile type>\r\n");
    user->message("    or " + cmd
		  + " #<body num> <mobile type>\r\n");
    return;
  }

  mobtype = STRINGD->to_lower(mobtype);

  if(mobtype == "user") {
    user->message("I know you're an administrator, but it's a bad idea "
		  + "to create random\r\n"
		  + "  user mobiles.  I'm stopping you.\r\n");
    return;
  }

  body = MAPD->get_room_by_num(bodynum);
  if((bodynum <= 0) || !body) {
    user->message("You must supply an appropriate body number with a "
		  + "corresponding body object.\r\n" + "Failed.\r\n");
    return;
  }

  if(mobnum > 0) {
    if(MOBILED->get_mobile_by_num(mobnum)) {
      user->message("There is already a mobile #" + mobnum
		    + " registered!\r\n");
      return;
    }

    segown = OBJNUMD->get_segment_owner(mobnum / 100);
    if(segown && segown != MOBILED) {
      user->message("That number is in a segment reserved for "
		    + "non-mobiles!\r\n");
      return;
    }
  }

  if(!MOBILED->get_file_by_mobile_type(mobtype)) {
    user->message("MOBILED doesn't recognize type '" + mobtype
		  + "',\r\n  so you can't create one.\r\n");
    user->message("  Maybe you need to add it to the binder?\r\n");
    return;
  }

  mobile = MOBILED->clone_mobile_by_type(mobtype);
  if(!mobile)
    error("Can't clone mobile of type '" + mobtype + "'!");

  mobile->assign_body(body);

  MOBILED->add_mobile_number(mobile, mobnum);
  user->message("Added mobile #" + mobile->get_number() + ".\r\n");
}
