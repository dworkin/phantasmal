#include <phantasmal/lpc_names.h>
#include <phantasmal/log.h>

#include <gameconfig.h>

/* This is the GAME_PATH_SPECIAL object.  It determines which AUTO
   object is used for objects under all /usr/Blah directories other
   than /usr/common and /usr/System.  Note that scripts (files of the
   form /usr/Blah/script) automatically inherit the Script AUTO
   object.  This file can't override that. */

string path_special(string filename) {
  if(sscanf(filename, "/usr/game/rooms/%*s") == 1) {
    return INHERIT_CUSTOM_ROOM_AUTO;
  }

  return INHERIT_COMMON_AUTO;
}
