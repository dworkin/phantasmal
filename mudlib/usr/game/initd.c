#include <phantasmal/log.h>

#include <gameconfig.h>
#include <config.h>

static void load_sould(void);

static void create(void) {
  string throwaway;

  /* Set up ConfigD */
  CONFIGD->set_start_room(0);
  CONFIGD->set_meat_locker(1);

  /* Build game driver and set it */
  throwaway = catch (find_object(GAME_DRIVER) ? nil
		     : compile_object(GAME_DRIVER));
  if(find_object(GAME_DRIVER))
    CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/data/help");

  load_sould();

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
}

static void load_sould(void) {
  string file_tmp;

  file_tmp = read_file("/usr/game/sould.unq");
  SOULD->from_unq_text(file_tmp);
}
