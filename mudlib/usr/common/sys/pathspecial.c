#include <phantasmal/lpc_names.h>
#include <config.h>

private object game_path_object;

string path_special(string file) {
  string user, subdir;

  if(sscanf(file, "/usr/%s/%s/%*s", user, subdir) != 3) {
    return nil;
  }

  if(subdir == "script") {
    return INHERIT_SCRIPT_AUTO;
  }

  if(user == "System" || user == "common") {
    return nil;
  }

  if(game_path_object)
    return game_path_object->path_special(file);

  return nil;
}

void set_game_path_object(object new_obj) {
  if(previous_program() == CONFIGD)
    game_path_object = new_obj;
  else
    error("Only ConfigD can set the game_path_object!");
}
