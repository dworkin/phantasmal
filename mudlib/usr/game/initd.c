#include <phantasmal/log.h>

#include <gameconfig.h>
#include <config.h>

static void config_unq_file(void);
static void configure_from_unq(mixed* unq);
static void load_files(void);

static void create(void) {
  /* Load in configuration files and set data in the common and System
     directories */
  load_files();

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/data/help");
}

static void load_files(void) {
  string file_tmp;

  /* Configure the MUD.  Currently, just hardcode stuff. */

  CONFIGD->set_start_room(0);
  CONFIGD->set_meat_locker(1);

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
