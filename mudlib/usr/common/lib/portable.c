#include <config.h>

inherit obj OBJECT;
inherit cont CONTAINER;

/* portable.c

   Inherited by any portable type.
*/


/* The portflags field contains a set of boolean flags specific to
   portables */
#define PF_CONTAINER          1
#define PF_OPEN               2
#define PF_NO_DESC            4
int portflags;

static void create(varargs int clone) {
  obj::create(clone);
  cont::create(clone);
  if(clone) {
    portflags = 0;
  }
}

void destructed(varargs int clone) {
  int    index;
  mixed* objs;

  objs = objects_in_container();
  for(index = 0; index < sizeof(objs); index++) {
    remove_from_container(objs[index]);

    if(location)
      location->add_to_container(objs[index]);
  }

  cont::destructed(clone);
  obj::destructed(clone);
}

void upgraded(varargs int clone) {
  cont::upgraded(clone);
  obj::upgraded(clone);
}

void set_number(int new_num) {
  if(previous_program() == PORTABLED) {
    tr_num = new_num;
  } else error("Only PORTABLED can set portable numbers!");
}

int is_container(void) {
  return portflags & PF_CONTAINER ? 1 : 0;
}

int is_open(void) {
  return portflags & PF_OPEN ? 1 : 0;
}

int is_no_desc(void) {
  return portflags & PF_NO_DESC ? 1 : 0;
}

void set_container(int cont) {
  if(cont) {
    portflags |= PF_CONTAINER;
  } else {
    portflags ^= ~PF_CONTAINER;
  }
}

void set_open(int open) {
  if(open) {
    portflags |= PF_OPEN;
  } else {
    portflags ^= ~PF_OPEN;
  }
}

void set_no_desc(int nodesc) {
  if(nodesc) {
    portflags |= PF_NO_DESC;
  } else {
    portflags ^= ~PF_NO_DESC;
  }
}
