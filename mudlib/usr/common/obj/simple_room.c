#include <phrase.h>
#include <config.h>
#include <map.h>
#include <log.h>

inherit room ROOM;
inherit unq UNQABLE;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);
  if(clone) {
    bdesc = PHR("a room");
    gdesc = PHR("a room");
    ldesc = PHR("You see a room here.");
    edesc = nil;

    MAPD->add_room_object(this_object());
  }
}

void destructed(int clone) {
  room::destructed(clone);
  unq::destructed(clone);
  if(clone) {
    MAPD->remove_room_object(this_object());
  }
}

void upgraded(varargs int clone) {
  room::upgraded(clone);
  unq::upgraded(clone);
}

string to_unq_text(void) {
  return "~room{\n" + to_unq_flags() + "}\n";
}

void from_dtd_unq(mixed* unq) {
  int ctr, is_room;

  if(unq[0] != "room")
    error("Doesn't look like room data!");

  for (ctr = 0; ctr < sizeof(unq[1]); ctr++) {
    from_dtd_tag(unq[1][ctr][0], unq[1][ctr][1]);
  }
}


/* function which returns an appropriate error message if this object
 * isn't a container or isn't open
 */
private string is_open_cont(object user) {
  if (!is_container()) {
    if(!user) return "not container";
    return get_brief()->to_string(user) + " isn't a container!";
  }
  if (!is_open()) {
    if(!user) return "not open";
    return get_brief()->to_string(user) + " isn't open!";
  }
  return nil;
}


/*
 * overloaded room notification functions
 */

string can_get(object user, object mover, object new_env) {
  if(mobile) {
    if(!user) return "sentient being";
    return get_brief()->to_string(user)
      + " is a sentient being!  You can't pick them up.";
  }
  return nil;
}

string can_put(object user, object mover, object movee, object old_env) {
  return is_open_cont(user);
}

string can_remove(object user, object mover, object movee, object new_env) {
  return is_open_cont(user);
}

string can_enter(object user, object enter_object, int dir) {
  if(mobile)
    return "You can't enter a sentient being!  Don't be silly.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "not container";
      return get_brief()->to_string(user) + " isn't a container.";
    } else {
      return nil;
    }
  } else {
    return is_open_cont(user);
  }
} 

string can_leave(object user, object leave_object, int dir) {
  if(mobile)
    return "You can't leave a sentient being!"
      + "  In fact, you shouldn't even be here.";

  if (dir == DIR_TELEPORT) {
    if (!is_container()) {
      if(!user) return "not container";
      return get_brief()->to_string(user) + " isn't a container.";
    } else {
      return nil;
    }
  } else {
    return is_open_cont(user);
  }
}
