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
