#include <phantasmal/log.h>

#include <type.h>
#include <gameconfig.h>
#include <config.h>

static mixed* load_file_with_dtd(string file_path, string dtd_path);

static void config_unq_file(void);
static void configure_from_unq(mixed* unq);

static void set_up_scripting(void);
static void set_up_heart_beat(void);
static void load_sould(void);
static void load_tagd(void);

static void create(void) {
  LOGD->write_syslog("**** Starting Seas of Night, version "
		     + GAME_VERSION + " ****");

  /* Build game driver and set it */
  if(!find_object(GAME_DRIVER))
    compile_object(GAME_DRIVER);

  CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Load configuration for game driver */
  config_unq_file();

  /* Set up TagD */
  load_tagd();

  /* Tests a simple script */
  set_up_scripting();

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/usr/game/help");

  load_sould();

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
  call_other("/usr/game/script/test_script", "???");
}

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

/* Parse UNQ in config.unq */
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
