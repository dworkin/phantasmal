#include <config.h>
#include <log.h>

/* container.c:

   Inherited by any object meant to contain other objects such as a
   jar, bag, basket, room or world.
*/

static mixed* objects;
static mixed* mobiles;

/* Prototypes */
void prepend_to_container(object obj);
void append_to_container(object obj);


static void create(varargs int clone) {
  if(clone) {
    mobiles = ({ });
    objects = ({ });
  }
}

void destructed(int clone) {
  int index;

  /* Anything not removed from the container before it deletes itself
     goes away...  This is necessary to avoid later consistency
     failures.  Something that's both an OBJECT and a CONTAINER,
     since it has location, can just pop this stuff in its own
     location, avoiding both the "disappear into nothingness" problem
     and the retrieval difficulties that suggests. */

  for(index = 0; index < sizeof(objects); index++) {
    objects[index]->set_location(nil);
  }
}

void upgraded(varargs int clone) {

}

void add_to_container(object obj) {
  append_to_container(obj);
}

void prepend_to_container(object obj) {
  if(!obj)
    error("Can't add (nil) to a container!");

  if(obj->get_location())
    error("Remove from previous container before adding!");

  obj->set_location(this_object());
  if(obj->get_mobile()) {
    mobiles += ({ obj->get_mobile() });
    obj->get_mobile()->notify_moved(obj);
  }

  objects = ({ obj }) + objects;
}

void append_to_container(object obj) {
  if(!obj)
    error("Can't add (nil) to a container!");

  if(obj->get_location())
    error("Remove from previous container before adding!");

  obj->set_location(this_object());
  if(obj->get_mobile()) {
    mobiles += ({ obj->get_mobile() });
    obj->get_mobile()->notify_moved(obj);
  }

  objects += ({ obj });
}

void remove_from_container(object obj) {
  if(obj->get_location() != this_object()) {
    error("Trying to remove object from wrong container!");
  }

  if(objects & ({ obj }) ) {
    objects -= ({ obj });
  } else {
    LOGD->write_syslog("Can't remove object from container!", LOG_ERR);
  }

  obj->set_location(nil);
  if(obj->get_mobile()) {
    mobiles -= ({ obj->get_mobile() });
    obj->get_mobile()->notify_moved(obj);
  }
}

object object_num_in_container(int num) {
  if(num < 0 || num >= sizeof(objects))
    return nil;

  return objects[num];
}

mixed* objects_in_container(void) {
  return objects[..];
}

mixed* mobiles_in_container(void) {
  return mobiles[..];
}

int num_objects_in_container(void) {
  return sizeof(objects);
}
