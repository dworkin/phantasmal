#include <config.h>
#include <kernel/kernel.h>

/* Mobile:  structure for a sentient, not-necessarily-player critter's
   mind.  The mobile will also be attached to a body under any
   normal circumstances.
*/

object body;     /* The mobile's body -- an OBJECT of some type */
object user;     /* The mobile's SYSTEM_USER, or nil */

static void create(varargs int clone) {
  if(clone) {

  }
}


void assign_body(object new_body) {
  if(!SYSTEM()) {
    error("Only SYSTEM objects can assign a mobile a new body!");
  }

  if(body) {
    body->set_mobile(nil);
    body = nil;
  }

  if(new_body) {
    new_body->set_mobile(this_object());
  }

  body = new_body;
}

object get_user(void) {
  return user;
}

void set_user(object new_user) {
  if(!SYSTEM()) {
    error("Only SYSTEM objects can assign a mobile a new user!");
  }

  user = new_user;
}

void notify_moved(object obj) {
  if(user) {
    user->notify_moved(obj);
  }
}
