#include <kernel/kernel.h>

#include <phantasmal/timed.h>
#include <phantasmal/log.h>

#include <config.h>
#include <type.h>

mapping* per_queue;
mixed*   per_call_out;

/***********************************************************************
 * TIMED handles call_outs on behalf of MUD mobiles and objects.
 * There is a small maximum total number of call_outs available, and
 * this uses them sparingly.  TimeD also gives a heartbeat function
 * interface, which is more familiar to long-time LPC users.
 ***********************************************************************/

private mixed* delay_tab;

/* Prototypes */
void upgraded(varargs int clone);
private void priv_start_call_out(int how_often);
private void priv_stop_call_out(int how_often);


static void create(void) {
  upgraded();
}

void upgraded(varargs int clone) {
  mixed *tmp_queue, *tmp_call_outs;
  int    size, ctr;

  if(!SYSTEM() && !COMMON())
    return;

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
  delay_tab[TIMED_HALF_MINUTE] = 30;
  delay_tab[TIMED_TEN_MINUTES] = 600;
  delay_tab[TIMED_ONE_HOUR] = 3600;
  delay_tab[TIMED_ONE_DAY] = 3600 * 24;
}

void set_heart_beat(int how_often, string funcname, mixed args...) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if((how_often >= TIMED_HIGHEST) || (how_often <= 0)) {
    error("Illegal value for how_often in TIMED::periodic_call_out!");
  }

  if(per_queue[how_often][object_name(previous_object())]) {
    error("Already have a heart_beat registered!  Unregister it first!");
  }

  LOGD->write_syslog("Setting up periodic call_out in TIMED", LOG_VERBOSE);
  per_queue[how_often][object_name(previous_object())]
    = ({ funcname, args });
  if(per_call_out[how_often] <= 0) {
    priv_start_call_out(how_often);
  }
}

private void stop_object_call_out(int how_often, string objname) {
  per_queue[how_often][objname] = nil;
  if(map_sizeof(per_queue[how_often]) == 0) {
    priv_stop_call_out(how_often);
  }
}

void stop_heart_beat(int how_often) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  stop_object_call_out(how_often, object_name(previous_object()));
}

private void priv_start_call_out(int how_often) {
  if(per_call_out[how_often] > 0) {
    LOGD->write_syslog("Call_out #" + how_often
		       + " is already started in TIMED::priv_start_call_out!",
		       LOG_WARNING);
    return;
  }
  if(delay_tab[how_often] && delay_tab[how_often] > 0) {
    per_call_out[how_often] = call_out("__priv_co_hook", delay_tab[how_often],
				       how_often);
  } else {
    LOGD->write_syslog("Trying to schedule zero-time callout!", LOG_ERR);
  }

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
  object call_obj;

  if(!KERNEL()) {
    error("TIMED::__priv_co_hook can be called only by KERNEL code!");
  }

  /* Schedule the next call */
  per_call_out[how_often] = 0;
  priv_start_call_out(how_often);

  keys = map_indices(per_queue[how_often]);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    tmp = per_queue[how_often][keys[ctr]];
    call_obj = find_object(keys[ctr]);
    if(call_obj) {
      call_other(call_obj, tmp[0], tmp[1]...);
    } else {
      LOGD->write_syslog("Can't find object " + keys[ctr] + " to call!",
			 LOG_WARN);
      stop_object_call_out(how_often, keys[ctr]);
    }
  }
}
