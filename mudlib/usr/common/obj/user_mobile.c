#include <config.h>
#include <kernel/kernel.h>
#include <map.h>

inherit MOBILE;

/* user associated with this mobile */
object user;

static void create(varargs int clone) {
  ::create(clone);
}

/* overridden functions from mobile.c */
string get_type(void) {
  return "user";
}

object get_user(void) {
  return user;
}

void set_user(object new_user) {
  if (!SYSTEM()) {
    error("Only SYSTEM objects can change the user!");
  }

  user = new_user;
}

void notify_moved(object obj) {
  if (user) {
    user->notify_moved(obj);
  }

  ::notify_moved(obj);
}

/* 
 * Hook function overrides
 */

void hook_say(object body, string message) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" says: " + message + "\r\n");
  }
}

void hook_emote(object body, string message) {
  if(user) {
    user->send_phrase(body->get_brief());
    user->message(" " + message + "\r\n");
  }
}

/* Args -- body, target, verb */
void hook_social(object body, object target, string verb) {
  string ret;

  if(user) {
    ret = SOULD->get_social_string(user, body, target, verb);
    if(!ret) {
      user->message("Error getting social string!\r\n");
    } else {
      user->message(ret + "\r\n");
    }
  }
}

void hook_whisper(object body, string message) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" whispers to you: " + message + "\r\n");
  }
}

void hook_whisper_other(object body, object target) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" whispers something to ");
    user->send_phrase(target->get_brief());
    user->message("\r\n");
  }
}

void hook_ask(object body, string message) {
  if (user) {
    user->send_phrase(body->get_brief());
    user->message(" asks you: " + message + "\r\n");
  }
}

void hook_ask_other(object body, object target, string message) {
  if (user) {
    if (target) {
      user->send_phrase(body->get_brief());
      user->message(" asks ");
      user->send_phrase(target->get_brief());
      user->message(": " + message + "\r\n");
    } else {
      user->send_phrase(body->get_brief());
      user->message(" asks: " + message + "\r\n");
    }
  }
}

void hook_leave(object body, int dir) {
  if (user) {
    if (dir == DIR_TELEPORT) {
      user->send_phrase(body->get_brief());
      user->message(" disappears into thin air.\r\n");
    } else {
      user->send_phrase(body->get_brief()) ;
      user->message(" leaves going ");
      user->send_phrase(EXITD->get_name_for_dir(dir));
      user->message("\r\n");
    }
  }
}

void hook_enter(object body, int dir) {
  if (user) {
    if (dir == DIR_TELEPORT) {
      user->send_phrase(body->get_brief());
      user->message(" appears from thin air.\r\n");
    } else {
      user->send_phrase(body->get_brief()) ;
      user->message(" enters from the ");
      user->send_phrase(EXITD->get_name_for_dir(dir));
      user->message("\r\n");
    }
  }
}

void from_dtd_unq(mixed* unq) {
  /* Set the body, location and number fields */
  unq = mobile_from_dtd_unq(unq);

  /* User mobiles don't actually (yet) have any additional data.
     So we can just return. */
}
