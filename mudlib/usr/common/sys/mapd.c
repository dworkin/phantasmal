#include <config.h>
#include <type.h>
#include <log.h>

#include <kernel/kernel.h>

#include <map.h>

/* The Mapd keeps track of room objects, their groupings and their
   relationship to each other.  It also loads in rooms in
   fundamentally data-based formats rather than code-based formats and
   turns them into proper room objects. */

/* room_objects keeps track of rooms by object name and certain aliases */
private mapping room_objects;

/* zone_segments keeps track of the mapping of what segments contain
   what segments meant for rooms */
private int**   zone_segments;

private object  room_dtd;
private int     initialized;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

/* Prototypes */
object get_room_by_num(int num);
int* rooms_in_zone(int zone);
private int assign_room_to_zone(int num, object room, int zone);
private int assign_room_number(int num, object room);



static void create(varargs int clone) {
  int numzones, ctr;

  if(clone)
    error("Cloning mapd is not allowed!");

  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);
  if(!find_object(UNQ_DTD))
    compile_object(UNQ_DTD);
  if(!find_object(SIMPLE_ROOM))
    compile_object(SIMPLE_ROOM);

  room_objects = ([ ]);
  zone_segments = ({ });
  numzones = ZONED->num_zones();
  for(ctr = 0; ctr < numzones; ctr++) {
    zone_segments += ({ ({ }) });
  }

  initialized = 0;
}

void upgraded(varargs int clone) {
}

void init(string dtd) {
  if(!initialized) {
    room_dtd = clone_object(UNQ_DTD);
    room_dtd->load(dtd);
    initialized = 1;
  } else error("MAPD already initialized!");
}

void destructed(int clone) {
  mixed* rooms;
  int    numzones, riter, ziter;

  if(room_dtd)
    destruct_object(room_dtd);

  /* Now go through and destruct all rooms */
  numzones = ZONED->num_zones();
  for(ziter = 0; ziter < numzones; ziter++) {
    rooms = rooms_in_zone(ziter);
    for(riter = 0; riter < sizeof(rooms); riter++) {
      destruct_object(get_room_by_num(rooms[riter]));
    }
  }
}


void set_segment_zone(int segment, int newzone, int oldzone) {
  if(previous_program() != OBJNUMD)
    error("Only objnumd can notify MAPD of a segment zone change!");

  if(oldzone == -1)
    oldzone = 0;
  if(sizeof(zone_segments[oldzone] & ({ segment }))) {
    zone_segments[oldzone] -= ({ segment });
  }

  if(newzone == -1)
    error("Setting zone to illegal zone number!");

  zone_segments[newzone] += ({ segment });
}


void add_room_object(object room) {
  string name;

  name = object_name(room);
  if(room_objects[name])
    error("Room already registered in add_room_object!");

  room_objects[name] = room;
}

void add_room_to_zone(object room, int num, int req_zone) {
  int seg, allocated, zone;

  allocated = 0;

  if(!room_objects[object_name(room)])
    error("Adding num for unregistered object " + object_name(room) + "!");

  num = assign_room_to_zone(num, room, req_zone);
  if(num < 0) {
    error("Error assigning room number!");
  }

  seg = num / 100;
  zone = OBJNUMD->get_segment_zone(seg);
  if(zone != req_zone && req_zone != -1)
    error("Room assigned to unreasonable segment!  Wrong zone!");

  if(zone == -1)
    zone = 0;  /* Fix offset */

  /* Check to see if this is a newly-allocated segment */
  if(!sizeof( ({ seg }) & zone_segments[zone])) {
    if(OBJNUMD->get_segment_owner(seg))
      error("Segment already allocated for non-room use!");

    zone_segments[zone] += ({ seg });
  }

  room->set_number(num);
}

/* For backwards compatibility */
void add_room_number(object room, int num) {
  add_room_to_zone(room, num, 0);
}

void set_room_alias(string alias, object room) {
  if(room_objects[alias] && !SYSTEM())
    error("Alias '" + alias + "' already registered in add_room_alias!");

  room_objects[alias] = room;
}

object get_room(string name) {
  return room_objects[name];
}

void remove_room_object(object room) {
  string name;
  int    num;

  name = object_name(room);
  if(!room_objects[name]) {
    error("Removing room not in room_objects mapping!");
  }
  room_objects[name] = nil;

  num = room->get_number();
  room->set_room_number(-1);

}

void remove_room_alias(string name) {
  if(find_object(name))
    error("Name is an object, not an alias!");

  if(!room_objects[name]) {
    error("Removing alias not in room_objects mapping!");
  }
  room_objects[name] = nil;
}

object get_room_by_num(int num) {
  if(num < 0) return nil;

  /* TODO: replace check with new zone-checking */
  /*
  int seg;

  seg = num / 100;
  if(!sizeof(nozone_segments & ({ seg })))
    return nil;
  */

  return OBJNUMD->get_object(num);
}


/* Find appropriate room number in the requested zone */
private int assign_room_to_zone(int num, object room, int req_zone) {
  int    segnum, ctr, zone;
  string segown;

  if(num != -1) {
    segnum = num / 100;

    segown = OBJNUMD->get_segment_owner(segnum);
    if(segown && strlen(segown) && segown != MAPD) {
      LOGD->write_syslog("Can't allocate room number " + num
			 + " in non-MAPD segment!", LOG_WARN);
      return -1;
    }
    zone = OBJNUMD->get_segment_zone(segnum);
    if(zone != req_zone && req_zone != 0)
      error("Room number (#" + num
	    + ") not in requested zone (#" + req_zone
	    + ") in assign_room_to_zone!");

    OBJNUMD->allocate_in_segment(segnum, num, room);

    if(zone < 0) {
      error("Zone is less than zero in assign_room_to_zone(" + num
	    + ", <room>, " + req_zone + ")!");
    }

    if(!sizeof(zone_segments[zone] & ({ segnum }))) {
      zone_segments[zone] += ({ segnum });
    }
    return num;
  } else {
    zone = req_zone;

    for(ctr = 0; ctr < sizeof(zone_segments[zone]); ctr++) {
      num = OBJNUMD->new_in_segment(zone_segments[zone][ctr], room);
      if(num != -1)
	break;
    }
    if(num == -1) {
      segnum = OBJNUMD->allocate_new_segment();
      
      zone_segments[zone] += ({ segnum });
      num = OBJNUMD->new_in_segment(segnum, room);
    }

    return num;
  }
}


private int assign_room_number(int num, object room) {
  /* Assign as unzoned */
  return assign_room_to_zone(num, room, 0);
}


/* Take an UNQ description parsed with a DTD and add the appropriate room
   to mapd. */
private object add_struct_for_room(mixed* unq) {
  object room;
  int    num;

  room = clone_object(SIMPLE_ROOM);
  room->from_dtd_unq(unq);

  num = room->get_number();
  num = assign_room_number(num, room);
  if(num < 0) {
    error("Can't assign room number!");
  }
  room->set_number(num);

  if(!room_objects[object_name(room)])
    error("You forgot to register with MAPD in a room object def maybe?");

  return room;
}

private void resolve_parent(object room) {
  int    pending_parent;
  object parent;

  pending_parent = room->get_pending_parent();
  if(pending_parent != -1) {
    parent = MAPD->get_room_by_num(pending_parent);
    if(!parent) {
      parent = PORTABLED->get_portable_by_num(pending_parent);
    }
    if(!parent) {
      error("Can't find parent number (#" + pending_parent
	    + ") loading rooms!");
    }

    room->set_archetype(parent);
  }

}

private int resolve_location(object room) {
  int    pending;
  object container;

  pending = room->get_pending_location();
  if(pending != -1) {
    container = get_room_by_num(pending);
    if(!container) {
      return 0;
    }

    container->add_to_container(room);
  } else {
    container = get_room_by_num(0);  /* Else, add to The Void */
    if(!container)
      error("Can't find room# 0!  Panic!");

    container->add_to_container(room);
  }

  return 1;
}

void add_dtd_unq_rooms(mixed* unq, string filename) {
  int    iter;
  mixed* resolve_rooms;
  object room;

  if(!initialized)
    error("Can't add rooms to uninitialized mapd!");

  /* TODO: we'll need the filename for objectd notify dependencies
     -- we'll need to keep track of the fact that if that file
     changes, these room objects change. */

  iter = 0;
  resolve_rooms = ({ });
  while(iter < sizeof(unq)) {
    room = add_struct_for_room( ({ unq[iter], unq[iter + 1] }) );
    resolve_rooms += ({ room });
    iter += 2;
  }

  for(iter = 0; iter < sizeof(resolve_rooms); iter++) {
    resolve_parent(resolve_rooms[iter]);
  }

  while(sizeof(resolve_rooms)) {
    for(iter = 0; iter < sizeof(resolve_rooms);) {
      if(resolve_location(resolve_rooms[iter])) {
	resolve_rooms = resolve_rooms[..iter-1] + resolve_rooms[iter+1..];
      } else {
	iter++;
      }
    }
  }
}

/* Take a chunk of text to parse as UNQ and add rooms appropriately... */
void add_unq_text_rooms(string text, string filename) {
  mixed* unq_data;

  if(!initialized)
    error("Can't add rooms to uninitialized mapd!");

  unq_data = UNQ_PARSER->unq_parse_with_dtd(text, room_dtd);
  if(!unq_data)
    error("Cannot parse text in add_unq_text_rooms!");

  add_dtd_unq_rooms(unq_data, filename);
}

int* segments_in_zone(int zone) {
  if(zone < 0 || zone >= sizeof(zone_segments)) {
    return nil;
  }

  return zone_segments[zone][..];
}

int* rooms_in_segment(int segment) {
  /* TODO: make this do new zone checking */
  /*
  if(sizeof( ({ segment }) & nozone_segments)) {
  }
  */

  return OBJNUMD->objects_in_segment(segment);
}

int* rooms_in_zone(int zone) {
  int *segs, *rooms, *tmp;
  int  iter;

  segs = segments_in_zone(zone);
  if(!segs) return nil;
  rooms = ({ });

  for(iter = 0; iter < sizeof(segs); iter++) {
    tmp = rooms_in_segment(segs[iter]);
    if(tmp)
      rooms += tmp;
  }

  return rooms;
}
