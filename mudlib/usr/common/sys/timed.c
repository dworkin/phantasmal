#include <config.h>
#include <log.h>
#include <type.h>
#include <timed.h>
#include <kernel/kernel.h>

mapping* per_queue;
mixed*   per_call_out;

/***********************************************************************
 * TIMED exists to keep track of real time, in-MUD time and
 * conversions between them.  It also handles call_outs on behalf of
 * MUD mobiles and objects that need them to avoid using up more than
 * necessary from the small total number of callouts.
 ***********************************************************************/

/* In-MUD time has a conversion factor associated with it -- that is,
   it's a multiple of real-world wall-clock time.  How long a
   real-world day is in the MUD (and vice-versa) depends on that
   conversion.

   However, there's a problem -- even with statedumps, when the MUD
   goes down, especially for a long time, there'll be a jump in
   real-world time.  The question is how the MUD should address that.
   Currently the TIMED simply keeps ticking away and the real-world
   time difference doesn't correspond to an in-MUD difference, but
   that may need to change in the future.

   Some ways we could deal with it:
     - Keep going forward, running all the call_outs, until we
       catch up.  That could be *really* slow on startup.
     - Cancel all the call_outs, but make some way to subscribe
       for a notification when the time skips to reschedule
       them.  That requires a lot of code on the part of
       anybody that uses TIMED to reschedule.
     - Keep going like nothing happened -- this would require
       any events that really cared about real-world time to
       check the time for themselves.
     - Have some notifications that run at specific times
       relative to the real world, and others that just run
       "periodically" -- the period ones would keep ticking away
       across statedumps.  The real-world ones would be
       delivered at the right times, but any that would have
       happened when the MUD was down would all happen at once
       right after it came back up, probably passing a "late"
       flag to say that they hadn't happened at the correct
       time.

  -angelbob
*/

private mixed* delay_tab;

/* Prototypes */
void upgraded(varargs int clone);
private void priv_start_call_out(int how_often);
private void priv_stop_call_out(int how_often);


static void create(varargs int clone) {
  if(clone) {
    error("Can't clone TIMED!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  mixed *tmp_queue, *tmp_call_outs;
  int    size, ctr;

  /* Allocate or reallocate the queue of periodic
     call_outs and the call_out numbers if necessary. */
  if(!per_queue || sizeof(per_queue) != TIMED_HIGHEST) {
    tmp_queue = allocate(TIMED_HIGHEST);
    tmp_call_outs = allocate(TIMED_HIGHEST);

    if(per_queue && (sizeof(per_queue) < TIMED_HIGHEST)) {
      size = sizeof(per_queue);
    } else if(per_queue) {
      size = TIMED_HIGHEST;
    } else {
      size = 0;
    }

    for(ctr = 0; ctr < size; ctr++) {
      tmp_queue[ctr] = per_queue[ctr];
      tmp_call_outs[ctr] = per_call_out[ctr];
    }

    /* Initialize any extra slots as empty */
    for(ctr = size; ctr < TIMED_HIGHEST; ctr++) {
      tmp_queue[ctr] = ([ ]);
      tmp_call_outs[ctr] = 0;
    }

    per_queue = tmp_queue;
    per_call_out = tmp_call_outs;
  }

  /* Hardcode a MUD minute to 20 real seconds right now,
     just to test. */
  delay_tab = allocate(TIMED_HIGHEST);
  delay_tab[TIMED_MUD_MINUTE] = 20;
}

void periodic_call_out(int how_often, string funcname, mixed args...) {
  if((how_often >= TIMED_HIGHEST) || (how_often <= 0)) {
    error("Illegal value for how_often in TIMED::periodic_call_out!");
  }

  LOGD->write_syslog("Setting up periodic call_out in TIMED");
  per_queue[how_often][object_name(previous_object())]
    = ({ funcname, args });
  if(per_call_out[how_often] <= 0) {
    priv_start_call_out(how_often);
  }
}

void stop_call_out(int how_often) {
  per_queue[how_often][object_name(previous_object())] = nil;
  if(map_sizeof(per_queue[how_often]) == 0) {
    priv_stop_call_out(how_often);
  }
}

private void priv_start_call_out(int how_often) {
  if(per_call_out[how_often] > 0) {
    LOGD->write_syslog("Call_out #" + how_often
		       + " is already started in TIMED::priv_start_call_out!",
		       LOG_WARNING);
    return;
  }
  per_call_out[how_often] = call_out("__priv_co_hook", delay_tab[how_often],
				     how_often);
  if(per_call_out[how_often] <= 0) {
    LOGD->write_syslog("Can't schedule call_out # " + how_often
		       + " in TIMED::priv_start_call_out!", LOG_ERROR);
    per_call_out[how_often] = 0;
  }
}

private void priv_stop_call_out(int how_often) {
  int ret;

  if(per_call_out[how_often] <= 0) {
    LOGD->write_syslog("Call_out #" + how_often
		       + " is already stopped in TIMED::priv_stop_call_out!",
		       LOG_WARNING);
    return;
  }
  ret = remove_call_out(per_call_out[how_often]);
  if(ret < 0) {
    LOGD->write_syslog("Call_out #" + how_often + ", handle #"
		       + per_call_out[how_often]
		       + " doesn't exist in TIMED::priv_stop_call_out!",
		       LOG_WARNING);
  }
  per_call_out[how_often] = 0;
}

void __priv_co_hook(int how_often) {
  int    ctr;
  mixed *keys, *tmp;

  if(!KERNEL()) {
    error("TIMED::__priv_co_hook can be called only by KERNEL code!");
  }

  LOGD->write_syslog("called __priv_co_hook...");

  /* Schedule the next call */
  per_call_out[how_often] = -1;
  priv_start_call_out(how_often);

  keys = map_indices(per_queue[how_often]);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    tmp = per_queue[how_often][keys[ctr]];
    call_other(keys[ctr], tmp[0], tmp[1]...);
  }
}
