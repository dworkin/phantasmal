#include <gameconfig.h>
#include <config.h>
#include <log.h>

static void config_unq_file(void);
static void configure_from_unq(mixed* unq);
static void load_files(void);

static void set_up_scripting(void);

static void create(void) {
  /* Load in configuration files and set data in the common and System
     directories */
  load_files();

  set_up_scripting();
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

static void load_files(void) {
  string file_tmp;

  /* Read config.unq file and add stuff to ConfigD */
  config_unq_file();

  file_tmp = read_file("/usr/game/text/welcome.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/welcome.msg!");
  CONFIGD->set_welcome_message(file_tmp);

  file_tmp = read_file("/usr/game/text/shutdown.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/shutdown.msg!");
  CONFIGD->set_shutdown_message(file_tmp);

  file_tmp = read_file("/usr/game/text/suspended.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/suspended.msg!");
  CONFIGD->set_suspended_message(file_tmp);

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
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
