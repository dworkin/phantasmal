#include <phantasmal/log.h>

#include <gameconfig.h>
#include <config.h>

static void config_unq_file(void);
static void configure_from_unq(mixed* unq);
static void load_files(void);

static void create(void) {
  string throwaway;

  /* Load in configuration files and set data in the common and System
     directories */
  load_files();

  /* Build game driver and set it */
  throwaway = catch (find_object(GAME_DRIVER) ? nil
		     : compile_object(GAME_DRIVER));
  if(find_object(GAME_DRIVER))
    CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/data/help");
}

static void load_files(void) {
  string file_tmp;

  /* Configure the MUD.  Currently, just hardcode stuff. */

  CONFIGD->set_start_room(0);
  CONFIGD->set_meat_locker(1);

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
}
