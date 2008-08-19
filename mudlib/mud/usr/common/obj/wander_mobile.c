#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

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
    LOGD->write_syslog("Setting up heart_beat in wander mobile",
		       LOG_VERBOSE);
    TIMED->set_heart_beat(TIMED_HALF_MINUTE, "__move_hook");
    registered = 1;
  }
}

string get_type(void) {
  return "wander";
}

void __move_hook(void) {
  int    num_ex, ctr;
  object exit, dest;
  string reason;

  if(previous_program() != TIMED) {
    error("wander_mobile::__move_hook should only be called by TIMED!");
  }

  num_ex = location->num_exits();
  if(num_ex > 0) {
    int dir;

    ctr = random(num_ex);
    exit = location->get_exit_num(ctr);
    if(!exit)
      error("Internal error!  Can't get exit!");

    dir = exit->get_direction();
    dest = exit->get_destination();

    reason = this_object()->move(dir);
    if(reason) {
      this_object()->say("I'm blocked!  I can't move there!");
      return;
    }
  }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Wandering mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
