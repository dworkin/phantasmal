#include <config.h>
#include <gameconfig.h>

/* This file exists so that the ObjectD can call to it to determine
   what AUTO file goes with what path. */

string path_special(string file) {
  string username;

  if(previous_program() == OBJECTD) {
    if(sscanf(file, "/usr/%s/script/%*s", username)) {
      if(username != "System") {
	SCRIPT_AUTO;
      } else
	return "";
    } else
      return "";
  } else {
    return nil;
  }
}
