#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <trace.h>
#include <type.h>
#include <status.h>
#include <limits.h>

#define SYSTEM_ROOMWIZTOOLLIB  "/usr/System/lib/room_wiztool"
#define SYSTEM_OBJWIZTOOLLIB   "/usr/System/lib/obj_wiztool"

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
    directory = USR_DIR + "/" + owner;
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

static void destructed(int clone) {
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


/*
 * NAME:	evaluate_lpc_code()
 * DESCRIPTION:	Evaluate a piece of LPC code, returning a result.
 *              Based on implementation of the code command.
 */
static mixed evaluate_lpc_code(object user, string lpc_code)
{
  mixed *parsed, result;
  object obj;
  string name, str;

  if (!lpc_code) {
    error("Can't evaluate (nil) as LPC code!");
  }

  parsed = parse_code(lpc_code);
  if (!parsed) {
    error("Couldn't parse code!");
  }
  name = USR_DIR + "/" + owner + "/_code";
  obj = find_object(name);
  if (obj) {
    destruct_object(obj);
  }

  str = USR_DIR + "/" + owner + "/include/code.h";
  if (file_info(str)) {
    str = "# include \"~/include/code.h\"\n";
  } else {
    str = "";
  }
  str = "# include <float.h>\n# include <limits.h>\n" +
    "# include <status.h>\n# include <trace.h>\n" +
    "# include <type.h>\n" + str + "\n" +
    "mixed exec(object user, mixed argv...) {\n" +
    "    mixed a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z;\n\n" +
    "    " + parsed[0] + "\n}\n";
  str = catch(obj = compile_object(name, str),
	      result = obj->exec(user, parsed[1 ..]...));
  if (str) {
    error(str);
    result = nil;
  }

  if (obj) {
    destruct_object(obj);
  }

  return result;
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
      phr = mob->get_body()->get_brief();
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
                + ZONED->get_segment_zone(ctr)
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
  ZONED->set_segment_zone(segnum, zonenum);

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

static void cmd_new_tag_type(object user, string cmd, string str) {
  string scope, name, type, getter, setter;
  mapping scope_strings, type_strings;

  scope_strings = ([ "object" : "object",
		   "obj" : "object",
		   "mobile" : "mobile",
		   "mob" : "mobile" ]);

  type_strings = ([ "int" : T_INT,
		  "1" : T_INT,
		  "integer" : T_INT,	
		  "float" : T_FLOAT,
		  "real" : T_FLOAT,
		  "2" : T_FLOAT ]);

  if(!str || !strlen(str) || (sscanf(str, "%*s %*s %*s %*s %*s %*s") == 6)
     || ((sscanf(str, "%s %s %s %s %s", scope, name, type, getter,
		 setter) != 5)
	 && (sscanf(str, "%s %s %s %s", scope, name, type, getter) != 4)
	 && (sscanf(str, "%s %s %s", scope, name, type) != 3))) {
    user->message("Usage: " + cmd
		  + " <obj|mob> <name> <type> [<getter> [<setter>]]\r\n");
    return;
  }

  scope = STRINGD->trim_whitespace(STRINGD->to_lower(scope));
  if(!scope_strings[scope]) {
    user->message("Scope '" + scope + "' isn't recognized.\r\n");
    user->message("Should be 'object' or 'mobile'.\r\n");
    return;
  }

  type = STRINGD->trim_whitespace(STRINGD->to_lower(type));
  if(!type_strings[type]) {
    user->message("Type '" + type + "' isn't recognized.\r\n");
    user->message("Should be 'int' or 'float'.\r\n");
    return;
  }

  name = STRINGD->trim_whitespace(name);

  if(getter)
    getter = STRINGD->trim_whitespace(getter);
  if(setter)
    setter = STRINGD->trim_whitespace(setter);

  switch(scope_strings[scope]) {
  case "object":
  case "mobile":
    call_other(TAGD, "new_" + scope_strings[scope] + "_tag",
	       name, type_strings[type], getter, setter);
    break;
  }
  user->message("Added new tag type '" + name + "'.\r\n");
}

static void cmd_set_tag(object user, string cmd, string str) {
  object obj_to_set;
  string usage_string, str2, err, tag_name;
  int    index;
  mixed  chk, *split_tmp;

  usage_string = "Usage: " + cmd + " #<obj> <tag name> <value>\r\n"
    + "       " + cmd + " $<hist> <tag name> <value>\r\n";
  if(sscanf(str, "#%d %s", index, str2) == 2) {
    obj_to_set = MAPD->get_room_by_num(index);
    if(!obj_to_set)
      obj_to_set = MOBILED->get_mobile_by_num(index);

    if(!obj_to_set) {
      user->message("Can't find object #" + index + "!\r\n" + usage_string);
      return;
    }
  } else if (sscanf(str, "$%d %s", index, str2) == 2) {
    if(index >= 0 && index <= sizeof(::query_history())) {
      chk = fetch(index);
      if(typeof(chk) == T_OBJECT)
	obj_to_set = chk;
      else {
	user->message("History entry $" + index + " isn't an object!\r\n");
	return;
      }

      if(function_object("get_tag", obj_to_set) != TAGGED) {
	user->message("History entry $" + index
		      + " isn't a tagged object!\r\n");
	return;
      }

    } else {
      user->message("Can't find history entry $" + index + ".\r\n");
      return;
    }

  } else {
    user->message(usage_string);
    return;
  }

  split_tmp = explode(str2, " ");
  if(sizeof(split_tmp) < 2) {
    user->message(usage_string);
    return;
  }
  tag_name = split_tmp[0];
  str2 = implode(split_tmp[1..], " ");

  user->message("Evaluating code w/ obj, '" + tag_name + "', code: '"
		+ str2 + "'.\r\n");

  /* Now we have obj_to_set, and str2 contains the remaining command line */
  /* err = catch (chk = evaluate_lpc_code(user, str2)); */
  chk = evaluate_lpc_code(user, str2);
  if(err) {
    user->message("Error evaluating code: " + err + "\r\n");
    return;
  }

  store(chk);
  TAGD->set_tag_value(obj_to_set, tag_name, chk);
  user->message("Set value of tag '" + tag_name + "' to "
		+ STRINGD->mixed_sprint(chk) + ".\r\n");
}

static void cmd_list_tags(object user, string cmd, string str) {
  string  *all_tags, msg;
  mapping  type_names;
  int      ctr;

  type_names = ([ T_INT : "int",
		T_FLOAT : "flt",
		T_MAPPING : "map",
		T_ARRAY : "arr",
		T_NIL   : "nil" ]);

  if(str)
    str = STRINGD->trim_whitespace(str);

  if(!str || !strlen(str)
     || ((str != "object") && (str != "mobile"))) {
    user->message("Usage: " + cmd + " <object|mobile>\r\n");
    return;
  }

  switch(str) {
  case "object":
    all_tags = TAGD->object_tag_names();
    break;
  case "mobile":
    all_tags = TAGD->mobile_tag_names();
    break;
  default:
    error("Internal error!");
  }

  if(sizeof(all_tags) == 0) {
    user->message("There are none.\r\n");
    return;
  }

  msg = "Tag Names & Types:\r\n";
  for(ctr = 0; ctr < sizeof(all_tags); ctr++) {
    int type;

    switch(str) {
    case "object":
      type = TAGD->object_tag_type(all_tags[ctr]);
      break;
    case "mobile":
      type = TAGD->mobile_tag_type(all_tags[ctr]);
      break;
    default:
      error("Internal error!");
    }

    msg += "  " + type_names[type] + "  " + all_tags[ctr] + "\r\n";
  }

  user->message_scroll(msg);
}
