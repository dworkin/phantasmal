#include <kernel/kernel.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/log.h>
#include <gameconfig.h>

/* This will eventually need to be upgraded to a mapping of mappings or
   the like.  In its current form it only holds a limited number of
   entries.  After four thousand rooms, boom! */
private mapping rooms;

void upgraded(varargs int clone) {
  if(!SYSTEM() && previous_program() != GAME_ROOM_REGISTRY)
    error("Only system code can call upgraded!");

  rooms = ([ ]);
}

static void create(varargs int clone) {
  upgraded();
}

void add_room(void) {
  string type;

  if(previous_program() != CUSTOM_ROOM_PARENT)
    error("Only privileged code can call room_registry:add_room!");

  if(sscanf(object_name(previous_object()), "/usr/game/obj/rooms/%s#%*d",
	    type) != 2)
    error("Can't parse object name in room_registry:add_room!");

  type = "/" + type;
  if(!rooms[type])
    rooms[type] = ({ });

  rooms[type] += ({ previous_object() });

  if(sizeof(rooms[type]) > 1)
    LOGD->write_syslog("Adding second object for obj type " + type + "!",
		       LOG_ERR);
}

object room_for_type(string type) {
  if(rooms[type]) {
    int i;

    for(i = 0; i < sizeof(rooms[type]); i++) {
      if(rooms[type][i]->get_number() > 0)
	return rooms[type][i];
    }
  }

  return nil;
}

int room_num_for_type(string type) {
  object room;

  room = room_for_type(type);
  if(room)
    return room->get_number();

  return -1;
}

string type_for_room_num(int num) {
  object room;
  string filename;

  room = MAPD->get_room_num(num);
  if(!room)
    return nil;

  if(sscanf(object_name(room), "/usr/game/rooms/%s#%*d", filename) != 2) {
    /* Can't parse object name */
    return nil;
  }

  return "/" + filename;
}
