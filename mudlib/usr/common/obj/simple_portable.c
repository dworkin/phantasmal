#include <kernel/kernel.h>
#include <config.h>
#include <map.h>

inherit obj ROOM;
inherit unq UNQABLE;

/* A simple portable object */

#define PHR(x) PHRASED->new_simple_english_phrase(x)

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
  ret += "}\n";
  return ret;
}

void from_dtd_unq(mixed* unq) {
  int ctr;

  if(unq[0] != "portable")
    error("Doesn't look like portable data!");

  /* load text */
  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    from_dtd_tag(unq[1][ctr][0], unq[1][ctr][1]);
  }
}


/* function which returns an appropriate error message if this object
 * isn't a container or open this one
 */

private string is_open_cont() {
  if (!is_container()) {
    return "That object isn't a container!";
  }
  if (!is_open()) {
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
    if (!is_container()) {
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
    if (!is_container()) {
      return "You can't enter that!";
    } else {
      return nil;
    }
  } else {
    return is_open_cont();
  }
}
