#include <config.h>

inherit EXIT;

/* A simple one-way exit between rooms. */

static void create(varargs int clone) {
  ::create(clone);
  if(clone) {

  }

}

void upgraded(varargs int clone) {
  ::upgraded(clone);
}

/* function which returns an appropriate error message if this object
 * isn't a container or isn't open
 */
private string is_open_cont() {
  if (!is_container()) {
    return "That object isn't a container!";
  }
  if (!is_open()) {
    return "That object isn't open!";
  }
  return nil;
}

