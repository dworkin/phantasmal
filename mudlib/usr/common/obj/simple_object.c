#include <config.h>

inherit OBJECT;

static void create(varargs int clone) {
  if(clone) {
    set_name("object");
  }
}
