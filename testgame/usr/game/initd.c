#include <phantasmal/lpc_names.h>
#include <phantasmal/log.h>

#include <type.h>
#include <gameconfig.h>
#include <config.h>
#include <version.h>

/* Normally an object with this path wouldn't need to explicitly
   include COMMON_AUTO, but this program gets compiled before the
   GAME_PATH_SPECIAL does.  And since it'll later get recompiled,
   thus inheriting COMMON_AUTO, it might as well start up with
   it active. */
inherit COMMON_AUTO;

static mixed* load_file_with_dtd(string file_path, string dtd_path);

static void config_unq_file(void);
static void configure_from_unq(mixed* unq);

static void set_up_scripting(void);
static void set_up_heart_beat(void);
static void load_sould(void);
static void load_tagd(void);
static void load_custom_rooms(void);
static int read_object_dir(string path);

static void create(void) {
  string mob_file;

  LOGD->write_syslog("**** Starting Test Game, "
		     + GAME_VERSION + " ****");

  /* GAME_PATH_SPECIAL should be compiled before any other object.  It
     changes the behavior of other objects because it sets their AUTO
     objects.  Rather than messing with all that nasty complexity,
     just compile this first so other objects in /usr/game always
     inherit from the same AUTO object(s). */
  if(!find_object(GAME_PATH_SPECIAL))
    compile_object(GAME_PATH_SPECIAL);
  CONFIGD->set_path_special_object(find_object(GAME_PATH_SPECIAL));

  /* Build game driver and set it */
  if(!find_object(GAME_DRIVER))
    compile_object(GAME_DRIVER);
  CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Find and compile the room registry *before* the first room is
     loaded.  Otherwise things will crash. */
  if(!find_object(GAME_ROOM_REGISTRY))
    compile_object(GAME_ROOM_REGISTRY);

  /* Load configuration for game driver */
  config_unq_file();

  /* Set up TagD */
  load_tagd();

  /* Tests a simple script */
  set_up_scripting();

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/usr/game/help");

  /* Load the SoulD with social commands */
  load_sould();

  compile_object(GAME_ROOM_BINDER);
  MAPD->set_binding_handler(find_object(GAME_ROOM_BINDER));

  /* Load stuff into MAPD and EXITD */
  if(read_object_dir(ROOM_DIR) >= 0) {
    EXITD->add_deferred_exits();
    MAPD->do_room_resolution(1);
  } else {
    LOGD->write_syslog("Can't read object files!  Starting incomplete!",
		       LOG_ERROR);
  }

  load_custom_rooms();

  /* Load the mobilefile into MOBILED */
  mob_file = read_file(MOB_FILE);
  if(mob_file) {
    MOBILED->add_unq_text_mobiles(mob_file, MOB_FILE);
  } else {
    LOGD->write_syslog("Can't read mobile file!  Starting w/o mobiles!",
		       LOG_ERROR);
  }

  /* Set up heart_beat functions */
  if(!find_object(HEART_BEAT))
    compile_object(HEART_BEAT);
  HEART_BEAT->set_up_heart_beat();

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
}

static mixed* load_file_with_dtd(string file_path, string dtd_path) {
  string file_tmp, dtd_tmp;
  object dtd;
  mixed* dtd_unq;

  dtd_tmp = read_file(dtd_path);
  file_tmp = read_file(file_path);

  dtd = clone_object(UNQ_DTD);
  dtd->load(dtd_tmp);

  dtd_unq = UNQ_PARSER->unq_parse_with_dtd(file_tmp, dtd, file_path);
  destruct_object(dtd);

  return dtd_unq;
}

static void set_up_scripting(void) {
  /* Test script obj */
  compile_object("/usr/game/script/test_script");
  call_other_unprotected("/usr/game/script/test_script", "???");
}

/* Read and parse config.unq according to config.dtd.  Then call
   config_from_unq to load the data into the Game Driver. */
static void config_unq_file(void) {
  object config_dtd;
  string config_dtd_file, config_file;
  mixed* unq_data;

  config_dtd_file = read_file("/usr/game/config.dtd");

  config_dtd = clone_object(UNQ_DTD);
  config_dtd->load(config_dtd_file);
  config_dtd_file = nil; /* Free the data */

  config_file = read_file("/usr/game/config.unq");

  unq_data = UNQ_PARSER->unq_parse_with_dtd(config_file, config_dtd);
  configure_from_unq(unq_data);

  destruct_object(config_dtd);
}

/* Parse UNQ data from config.unq, load it into Game Driver */
static void configure_from_unq(mixed* unq) {
  int set_sr, set_ml;

  set_sr = set_ml = 0;

  while(sizeof(unq) > 1) {
    if(unq[0] == "start_room") {
      if(set_sr)
	error("Duplicate start_room entry in MUD config file!");

      GAME_DRIVER->set_start_room(unq[1]);
      set_sr = 1;
    } else if (unq[0] == "meat_locker") {
      if(set_ml)
	error("Duplicate meat_locker entry in MUD config file!");

      GAME_DRIVER->set_meat_locker(unq[1]);
      set_ml = 1;
    } else {
      error("Unrecognized UNQ tag from DTD in MUD config file!");
    }

    unq = unq[2..];
  }

}

static void load_sould(void) {
  string file_tmp;

  file_tmp = read_file("/usr/game/sould.unq");
  SOULD->from_unq_text(file_tmp);
}

static void load_tagd(void) {
  mixed *dtd_unq;
  int    ctr, ctr2;

  dtd_unq = load_file_with_dtd("/usr/game/tagd.unq", "/usr/game/tagd.dtd");

  for(ctr = 0; ctr < sizeof(dtd_unq); ctr += 2) {
    string tag_name, tag_get, tag_set, add_func;
    int    tag_type;

    if(typeof(dtd_unq[ctr + 1]) != T_ARRAY)
      error("Internal error parsing TAGD file!");

    switch(dtd_unq[ctr]) {
    case "mobile_tag":
      add_func = "new_mobile_tag";
      break;
    case "object_tag":
      add_func = "new_object_tag";
      break;
    default:
      error("Unknown tag '" + STRINGD->mixed_sprint(dtd_unq[ctr]) + "'!");
    }

    tag_get = tag_set = nil;
    tag_type = -1;

    for(ctr2 = 0; ctr2 < sizeof(dtd_unq[ctr + 1]); ctr2++) {
      switch(dtd_unq[ctr + 1][ctr2][0]) {
      case "name":
	tag_name = STRINGD->trim_whitespace(dtd_unq[ctr + 1][ctr2][1]);
	break;
      case "type":
	tag_type = dtd_unq[ctr + 1][ctr2][1];
	break;
      case "getter":
	tag_get = STRINGD->trim_whitespace(dtd_unq[ctr + 1][ctr2][1]);
	break;
      case "setter":
	tag_set = STRINGD->trim_whitespace(dtd_unq[ctr + 1][ctr2][1]);
	break;
      default:
	error("Unrecognized label in switch for TagD UNQ: "
	      + STRINGD->mixed_sprint(dtd_unq[ctr + 1][ctr2]) + "!");
      }
    }

    call_other(TAGD, add_func, tag_name, tag_type, tag_get, tag_set);
  }
}

/* This function looks through the appropriate subdirectories for room
   files and sets them up with the MapD */
static void load_custom_rooms(void) {
  mixed **dir_list;
  int     ctr;
  string  err, prog_name;

  dir_list = get_dir(GAME_ROOMS_DIR . "*");
  for(ctr = 0; ctr < sizeof(dir_list[0]); ctr++) {
    if(dir_list[1][ctr] == -2) {
      /* TODO: Directory, recurse */
    } else if(sscanf(dir_list[0][ctr], "%s.c", prog_name) == 1) {
      /* Custom room file, make sure the room is used */
      if(!GAME_ROOM_REGISTRY->room_for_type("/" + prog_name)) {
	LOGD->write_syslog("Compiling room " + prog_name);
	/* Try to compile object for use. */
	compile_object(GAME_ROOMS_DIR + prog_name);
	if(find_object(GAME_ROOMS_DIR + prog_name)) {
	  object newobj;
	  /* Clone the object so it'll show up in the registry */
	  newobj = clone_object(GAME_ROOMS_DIR + prog_name);
	  if(newobj) {
	    MAPD->add_room_to_zone(newobj, -1, 0);
	  } else {
	    LOGD->write_syslog("Couldn't clone " + prog_name
			       + ", not enough memory?");
	  }
	}
      }
    }
  }
}

/* read_object_dir loads all rooms and exits from the specified directory,
   which should be in canonical Phantasmal saved format.  This is used to
   restore saved data from %shutdown and from %datadump. */
static int read_object_dir(string path) {
  mixed** dir;
  int     ctr;
  string  file;

  dir = get_dir(path + "/zone*.unq");
  if(!sizeof(dir[0])) {
    LOGD->write_syslog("Can't find any '" + path
		       + "/zone*.unq' files to load!", LOG_ERR);
    return -1;
  }

  for(ctr = 0; ctr < sizeof(dir[0]); ctr++) {
    /* Skip directories */
    if(dir[1][ctr] == -2)
      continue;

    file = read_file(path + "/" + dir[0][ctr]);
    if(!file || !strlen(file)) {
      /* Nothing was read.  Return error. */
      return -1;
    }

    MAPD->add_unq_text_rooms(file, ROOM_DIR + "/" + dir[0][ctr]);
  }
}
