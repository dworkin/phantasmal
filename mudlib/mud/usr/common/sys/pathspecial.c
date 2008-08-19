/*
 * pathspecial.c
 *
 * Once ConfigD has set this object up, the Object Daemon will query
 * it to determine the path-special objects for everything in the
 * system.  The idea is that this file hardcodes certain files as
 * having particular special AUTO objects, and then hands off to the
 * game's path-special object (set in GAME_INITD) for everything else
 * that isn't in /usr/common or /usr/System.

 * Currently, only files under a /usr/ directory may have a special
 * AUTO object.  The Kernel Library prevents files under /usr/System
 * from having one, though.
 */

#include <phantasmal/lpc_names.h>

inherit COMMON_AUTO;

private object game_path_object;

string path_special(string file) {
  string user, subdir, tmp;

  if(file == COMMON_AUTO)
    return nil;

  if(sscanf(file, "/usr/%s/%s/%*s", user, subdir) != 3) {
    return nil;
  }

  if(user != "System" && user != "common" && subdir == "script")
    return INHERIT_SCRIPT_AUTO;

  if(user == "common") {
    return INHERIT_COMMON_AUTO;
  }

  if(game_path_object) {
    tmp = game_path_object->path_special(file);
    if(!tmp || tmp == "")
      return INHERIT_COMMON_AUTO;

    return tmp;
  }

  return nil;
}

void set_game_path_object(object new_obj) {
  if(previous_program() == CONFIGD)
    game_path_object = new_obj;
  else
    error("Only ConfigD can set the game_path_object!");
}
