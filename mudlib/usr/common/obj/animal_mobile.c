#include <config.h>
#include <kernel/kernel.h>
#include <timed.h>
#include <log.h>

inherit MOBILE;

/* Member variables */
private int registered;

/* Inherited mobile data */
static int aggression;

/* prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  ::create(clone);

  upgraded(clone);
}


void upgraded(varargs int clone) {
  if(clone && !registered) {
    LOGD->write_syslog("Setting up periodic call_out in wander mobile",
		       LOG_VERBOSE);
    TIMED->periodic_call_out(TIMED_MUD_MINUTE, "__vol_hook");
    registered = 1;
  }

  if(clone) {
    /* This is mainly just an example at this point */
    aggression = 0;
  }
}

string get_type(void) {
  return "animal";
}

void __vol_hook(void) {
  int    num_ex, ctr;
  object exit, reason, dest;

  if(previous_program() != TIMED) {
    error("animal_mobile::__vol_hook should only be called by TIMED!");
  }

  if(random(100) < 70) {
    /* Do nothing for the moment */
    if(aggression && (random(100) < 25)) {
      social("growl", nil);
    }
    return;
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

static mixed *animal_from_dtd_unq(mixed* unq) {
  mixed *ret, *ctr;
  int    bodynum;

  ret = ({ });
  ctr = unq;

  while(sizeof(ctr) > 0) {
    if(!STRINGD->stricmp(ctr[0][0], "aggression")) {
      aggression = ctr[0][1];
    } else {
      ret += ({ ctr[0] });
    }
    ctr = ctr[1..];
  }

  return ret;
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* Now parse the stuff we care about */
  unq = animal_from_dtd_unq(unq);
}

string mobile_unq_fields(void) {
  return "  ~aggression{" + aggression + "}\n";
}
