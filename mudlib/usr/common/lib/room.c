#include <config.h>
#include <log.h>

/* room.c:

   A basic MUD room with standard trimmings
*/

/* inherit cont CONTAINER; */
inherit obj OBJECT;

/* The objflags field contains a set of boolean flags specific to
   portables */
#define OF_CONTAINER          1
#define OF_OPEN               2
#define OF_NO_DESC            4
int objflags;

mixed* exits;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  /* cont::create(clone); */
  obj::create(clone);
  if(clone) {
    exits = ({ });
  }
}

void destructed(int clone) {
  int index;
  mixed *objs;

  objs = objects_in_container();
  for(index = 0; index < sizeof(objs); index++) {
    remove_from_container(objs[index]);

    if(location)
      location->add_to_container(objs[index]);
  }

  /* cont::destructed(clone); */
  obj::destructed(clone);
}

void upgraded(varargs int clone) {
  /* cont::upgraded(); */
  obj::upgraded();
}

void set_number(int new_num) {
  if(previous_program() == MAPD
     || previous_program() == PORTABLED) {
    tr_num = new_num;
  } else error("Only MAPD or PORTABLED can set room numbers!");
}


/*** Object Flags functions ***/

int is_container(void) {
  return objflags & OF_CONTAINER ? 1 : 0;
}

int is_open(void) {
  return objflags & OF_OPEN ? 1 : 0;
}

int is_no_desc(void) {
  return objflags & OF_NO_DESC ? 1 : 0;
}

void set_container(int cont) {
  if(cont) {
    objflags |= OF_CONTAINER;
  } else {
    objflags ^= ~OF_CONTAINER;
  }
}

void set_open(int open) {
  if(open) {
    objflags |= OF_OPEN;
  } else {
    objflags ^= ~OF_OPEN;
  }
}

void set_no_desc(int nodesc) {
  if(nodesc) {
    objflags |= OF_NO_DESC;
  } else {
    objflags ^= ~OF_NO_DESC;
  }
}


/*** Functions dealing with Exits ***/

void clear_exits(void) {
  if(previous_program() == EXITD)
    exits = ({ });
  else error("Only EXITD can clear exits!");
}

void add_exit(int dir, object exit) {
  if(previous_program() == EXITD) {
    exits = exits + ({ ({ dir, exit }) });
  } else error("Only EXITD can add exits!");
}

int num_exits(void) {
  return sizeof(exits);
}

object get_exit_num(int index) {
  if(index < 0) return nil;
  if(index >= sizeof(exits)) return nil;

  return exits[index][1];
}

object get_exit(int dir) {
  int ctr;

  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    if(exits[ctr][0] == dir)
      return exits[ctr][1];
  }

  return nil;
}

void remove_exit(object exit) {
  if(previous_program() == EXITD) {
    int ctr;

    for(ctr = 0; ctr < sizeof(exits); ctr++) {
      if(exits[ctr][1] == exit) {
	exits = exits[..ctr-1] + exits[ctr+1..];
	return;
      }
    }

    LOGD->write_syslog("Can't find exit to remove [" + sizeof(exits)
		       + " exits]!", LOG_ERR);
  } else error("Only EXITD can remove exits!");
}
