#include <config.h>
#include <log.h>
#include <type.h>

inherit unq DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);


/* Data from config file*/
int start_room;
int meat_locker;


static void create(varargs int clone) {
  if(clone) {
    error("Can't clone CONFIGD!");
  }

  unq::create(clone);
  upgraded();
}

void upgraded(varargs int clone) {
  set_dtd_file(CONFIGD_DTD);
  unq::upgraded();

  /* We'll need to load the file's contents... */
  load_from_file(CONFIG_FILE);
}

mixed* to_dtd_unq(void) {
  return ({ ({ "start_room", start_room }),
	      ({ "meat_locker", meat_locker })
	      });
}

void from_dtd_unq(mixed* unq) {
  int set_sr, set_ml;

  start_room = meat_locker = 0;
  set_sr = set_ml = 0;

  while(sizeof(unq) > 1) {
    if(unq[0] == "start_room") {
      if(set_sr && unq[1] != start_room)
	error("Duplicate start_room entry in CONFIGD's file!");

      start_room = unq[1];
      set_sr = 1;
    } else if (unq[0] == "meat_locker") {
      if(set_ml && unq[1] != meat_locker)
	error("Duplicate meat_locker entry in CONFIGD's file!");

      meat_locker = unq[1];
      set_ml = 1;
    } else {
      error("Unrecognized UNQ tag from DTD in CONFIGD!");
    }

    unq = unq[2..];
  }

}

int get_start_room(void) {
  return start_room;
}

int get_meat_locker(void) {
  return meat_locker;
}
