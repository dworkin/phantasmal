#include <kernel/kernel.h>
#include <config.h>
#include <map.h>

inherit obj ROOM;
inherit unq UNQABLE;

/* A simple portable object */

#define PHR(x) PHRASED->new_simple_english_phrase(x)

/* The objflags field contains a set of boolean object flags */
#define OF_CONTAINER          1
#define OF_OPEN               2
#define OF_NODESC             4
#define OF_OPENABLE           8

int objflags;

static void create(varargs int clone) {
  obj::create(clone);
  unq::create(clone);
  if(clone) {
    set_brief(PHR("A portable object"));
    set_glance(PHR("A new portable object"));
    set_look(PHR("Why look, it's an object.  Portable, but uninitialized!"));

    MAPD->add_room_object(this_object());
  }

}

void upgraded(varargs int clone) {
  obj::upgraded(clone);
  unq::upgraded(clone);
}

void destructed(varargs int clone) {
  obj::destructed(clone);
  unq::destructed(clone);
}

string to_unq_text(void) {
  string ret;
  ret = "~portable{\n";
  ret += to_unq_flags();
  ret += "~flags{" + objflags + "}\n";
  ret += "}\n";
  return ret;
}

void from_dtd_unq(mixed* unq) {
  int ctr;

  if(unq[0] != "portable")
    error("Doesn't look like portable data!");

  /* load text */
  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    if (unq[1][ctr][0] == "flags") {
      objflags = unq[1][ctr][1];
    } else {
      from_dtd_tag(unq[1][ctr][0], unq[1][ctr][1]);
    }
  }
}

/* 
 * flag overrides
 */

int is_no_desc() {
  return objflags & OF_NODESC;
}

int is_container() {
  return objflags & OF_CONTAINER;
}

int is_open() {
  return objflags & OF_OPEN;
}

int is_openable() {
  return objflags & OF_OPENABLE;
}

private void set_flags(int flags, int value) {
  if(value) {
    objflags |= flags;
  } else {
    objflags &= ~flags;
  }
}

void set_nodesc(int value) {
  if(!SYSTEM())
    error("Only SYSTEM code can currently set an object nodesc!");

  set_flags(OF_NODESC, value);
}

void set_container(int value) {
  if(!SYSTEM())
    error("Only SYSTEM code can currently set an object as a container!");

  set_flags(OF_CONTAINER, value);
}

void set_open(int value) {
  if(!SYSTEM())
    error("Only SYSTEM code can currently set an object as open!");

  set_flags(OF_OPEN, value);
}

void set_openable(int value) {
  if(!SYSTEM())
    error("Only SYSTEM code can currently set an object as openable!");

  set_flags(OF_OPENABLE, value);
}

/* function which returns an appropriate error message if this object
 * isn't a container or open this one
 */

private string is_open_cont() {
  if (!(objflags & OF_CONTAINER)) {
    return "That object isn't a container!";
  }
  if (!(objflags & OF_OPEN)) {
    return "That object isn't open!";
  }
  return nil;
}

/*
 * overloaded room notification functions
 */

string can_put(object mover, object movee, object old_env) {
  return is_open_cont();
}

string can_remove(object mover, object movee, object new_env) {
  return is_open_cont();
}

string can_enter(object enter_object, int dir) {
  if (dir == DIR_TELEPORT) {
    if (!(objflags & OF_CONTAINER)) {
      return "You can't enter that!";
    } else {
      return nil;
    }
  } else {
    return is_open_cont();
  }
} 

string can_leave(object leave_object, int dir) {
  if (dir == DIR_TELEPORT) {
    if (!(objflags & OF_CONTAINER)) {
      return "You can't enter that!";
    } else {
      return nil;
    }
  } else {
    return is_open_cont();
  }
}
