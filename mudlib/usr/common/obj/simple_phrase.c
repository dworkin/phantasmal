#include <config.h>

inherit intl INTL_PHRASE;

static void create(varargs int clone) {
  if(clone) {
    intl::create(clone);
  }
}
