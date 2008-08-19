#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>
#include <gameconfig.h>
#include <config.h>

static void load_sould(void);
static int read_object_dir(string path);

static void create(void) {
  string throwaway, mob_file;

  /* Build game driver and set it */
  throwaway = catch (find_object(GAME_DRIVER) ? nil
		     : compile_object(GAME_DRIVER));
  if(find_object(GAME_DRIVER))
    CONFIGD->set_game_driver(find_object(GAME_DRIVER));

  /* Register a help directory for the HelpD to use */
  HELPD->new_help_directory("/usr/game/help");

  load_sould();

  /* Load stuff into MAPD and EXITD */
  if(read_object_dir(ROOM_DIR) >= 0) {
    EXITD->add_deferred_exits();
    MAPD->do_room_resolution(1);
  } else {
    error("Can't read object files!  Dying!\n");
  }

  /* Load the mobilefile into MOBILED */
  mob_file = read_file(MOB_FILE);
  if(mob_file) {
    MOBILED->add_unq_text_mobiles(mob_file, MOB_FILE);
  } else {
    error("Can't read mobile file!  Dying!\n");
  }

  LOGD->write_syslog("Configured Phantasmal from /usr/game!");
}

static void load_sould(void) {
  string file_tmp;

  file_tmp = read_file("/usr/game/sould.unq");
  SOULD->from_unq_text(file_tmp);
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
