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

  upgraded();
}


void upgraded(varargs int clone) {
  if(!registered) {
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

  LOGD->write_syslog("Yup, it worked.");
}
