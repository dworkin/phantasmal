#include <config.h>
#include <log.h>

static void configure_from_file(void);
static void configure_from_unq(mixed* unq);

void create(void) {
  configure_from_file();
}

static void configure_from_file(void) {
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

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
}

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
