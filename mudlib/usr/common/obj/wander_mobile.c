#include <config.h>
#include <kernel/kernel.h>
#include <timed.h>

inherit MOBILE;

/* Member variables */
private int registered;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone) {
  if(clone && !registered) {
    LOGD->write_syslog("Setting up periodic call_out in wander mobile");
    TIMED->periodic_call_out(TIMED_MUD_MINUTE, "__move_hook");
    registered = 1;
  }
}

string get_type(void) {
  return "wander";
}

void __move_hook(void) {
  if(previous_program() != TIMED) {
    error("wander_mobile::__move_hook should only be called by TIMED!");
  }

  LOGD->write_syslog("A wander_mobile should move randomly now.");
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
