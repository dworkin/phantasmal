#include <phantasmal/log.h>

#include <gameconfig.h>
#include <config.h>

static void config_unq_file(void);
static void configure_from_unq(mixed* unq);

static void set_up_scripting(void);
static void set_up_heart_beat(void);
static void load_sould(void);

static void create(void) {
  /* Load in configuration files and set data in the common and System
     directories */
  config_unq_file();

  set_up_scripting();

  /* Build game driver and set it */
  if(!find_object(GAME_DRIVER))
    compile_object(GAME_DRIVER);

  CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/usr/game/help");

  load_sould();

  /* Set up heart_beat functions */
  if(!find_object(HEART_BEAT))
    compile_object(HEART_BEAT);
  HEART_BEAT->set_up_heart_beat();

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
}

static void set_up_scripting(void) {
  /* Set up special AUTO paths for scripts */
  if(!find_object(PATHAUTOD))
    compile_object(PATHAUTOD);

  compile_object(SCRIPT_AUTO_OBJECT);

  CONFIGD->set_path_special_object(find_object(PATHAUTOD));

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

      CONFIGD->set_start_room(unq[1]);
      set_sr = 1;
    } else if (unq[0] == "meat_locker") {
      if(set_ml)
	error("Duplicate meat_locker entry in MUD config file!");

      CONFIGD->set_meat_locker(unq[1]);
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
