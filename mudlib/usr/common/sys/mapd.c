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

/* which unq tags are mapped to which lpc code */
private mapping tag_code;

/* zone_segments keeps track of the mapping of what segments contain
   what segments meant for rooms */
private int**   zone_segments;

private object  room_dtd, bind_dtd;
private int     initialized;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

/* Prototypes */
object get_room_by_num(int num);
int* rooms_in_zone(int zone);
private int assign_room_to_zone(int num, object room, int zone);
private int assign_room_number(int num, object room);
void upgraded(varargs int clone);


static void create(varargs int clone) {
  if(clone)
    error("Cloning mapd is not allowed!");

  room_objects = ([ ]);
  tag_code = ([ ]);

  zone_segments = ({ });
  upgraded(clone);

  initialized = 0;
}

void upgraded(varargs int clone) {
  int numzones, size, ctr;

  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);
  if(!find_object(UNQ_DTD))
    compile_object(UNQ_DTD);

  numzones = ZONED->num_zones();
  size = sizeof(zone_segments);

  if(size != numzones) {
    if(size > numzones)
      error("We can't handle dynamically reducing the number of zones!");

    for(ctr = 0; ctr < numzones - size; ctr++) {
      LOGD->write_syslog("Adding zone in MAPD!", LOG_VERBOSE);
      zone_segments += ({ ({ }) });
    }
  }
}

void init(string room_dtd_str, string bind_dtd_str) {
  int    ctr, numzones;
  mixed *unq_data;
  string bind_file, tag, file;

  if(!initialized) {
    room_dtd = clone_object(UNQ_DTD);
    room_dtd->load(room_dtd_str);

    /* read the binder file */
    bind_dtd = clone_object(UNQ_DTD);
    bind_dtd->load(bind_dtd_str);

    bind_file = read_file(ROOM_BIND_FILE);
    if (!bind_file)
      error("Cannot read binder file " + ROOM_BIND_FILE + "!");

    unq_data = UNQ_PARSER->unq_parse_with_dtd(bind_file, bind_dtd);
    if(!unq_data)
      error("Cannot parse binder text in MAPD::init()!");

    if (sizeof(unq_data) % 2)
      error("Odd sized unq chunk in MAPD::init()!");

    for (ctr = 0; ctr < sizeof(unq_data); ctr += 2) {
      if (STRINGD->stricmp(unq_data[ctr],"bind"))
	error("Not a code/tag binding in MAPD::init()!");

      if (typeof(unq_data[ctr+1]) != T_ARRAY || sizeof(unq_data[ctr+1]) != 2) {
	/* Should never get here for proper DTD */
	error("Internal error in MAPD->init()");
      }


      if (!STRINGD->stricmp(unq_data[ctr+1][0][0],"tag")) {
	tag = unq_data[ctr+1][0][1];
	file = unq_data[ctr+1][1][1];
      } else {
	tag = unq_data[ctr+1][1][1];
	file = unq_data[ctr+1][0][1];
      }

      if (tag_code[tag] != nil) {
	error("Tag " + tag + " is already bound in MAPD::init()!");
      }

      /* Assign file to tag, and make sure it exists and is clonable */
      tag_code[tag] = file;
      if(!find_object(file))
	compile_object(file);
    }

    initialized = 1;
  } else error("MAPD already initialized!");
}

void destructed(int clone) {
  mixed* rooms;
  int    numzones, riter, ziter;

  if(room_dtd)
    destruct_object(room_dtd);
  if(bind_dtd)
    destruct_object(bind_dtd);

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

  if(newzone < 0)
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
  error("Obsolete function!");
  add_room_to_zone(room, num, 0);
}

void set_room_alias(string alias, object room) {
  error("Obsolete function!");

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
  error("Obsolete function!");

  if(find_object(name))
    error("Name is an object, not an alias!");

  if(!room_objects[name]) {
    error("Removing alias not in room_objects mapping!");
  }
  room_objects[name] = nil;
}

object get_room_by_num(int num) {
  if(num < 0) return nil;

  return OBJNUMD->get_object(num);
}


/* Find appropriate room number in the requested zone */
private int assign_room_to_zone(int num, object room, int req_zone) {
  int    segnum, ctr, zone;
  string segown;

  if(num >= 0) {
    segnum = num / 100;

    segown = OBJNUMD->get_segment_owner(segnum);
    if(segown && strlen(segown) && segown != MAPD) {
      LOGD->write_syslog("Can't allocate room number " + num
			 + " in non-MAPD segment!", LOG_WARN);
      return -1;
    }
    zone = OBJNUMD->get_segment_zone(segnum);
    if(zone != req_zone && req_zone >= 0)
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

  /* no unq passed in, so no object passed out */
  if (sizeof(unq) == 0) {
    return nil;
  }

  if (tag_code[unq[0]] == nil) {
    error("Tag " + unq[0] + " not bound to any code!");
  }

  if (!find_object(tag_code[unq[0]])) {
    compile_object(tag_code[unq[0]]);
  }

  room = clone_object(tag_code[unq[0]]);
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
      error("Can't find parent number (#" + pending_parent
	    + ") loading rooms!");
    }

    room->set_archetype(parent);
  }

}

private void resolve_removed_details(object room) {
  int    *rem_det;
  int     ctr;
  object  tmp;
  object *new_rem_det;

  rem_det = room->get_pending_removed_details();
  new_rem_det = ({ });
  for(ctr = 0; ctr < sizeof(rem_det); ctr++) {
    tmp = MAPD->get_room_by_num(rem_det[ctr]);
    if(!tmp)
      error("Can't resolve removed details!");

    new_rem_det += ({ tmp });
  }

  room->set_removed_details(new_rem_det);
}

private int resolve_location(object room) {
  int    pending;
  object container;

  /* Resolve details (rather than regular containment) */
  pending = room->get_pending_detail_of();
  if(pending != -1) {
    container = get_room_by_num(pending);
    if(!container) {
      return 0;
    }

    container->add_detail(room);
    return 1;
  }

  /* Resolve regular containment */
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
      error("Can't find room #0!  Panic!");

    container->add_to_container(room);
  }

  return 1;
}

void add_dtd_unq_rooms(mixed* unq, string filename) {
  int    iter, res_tmp;
  mixed* resolve_rooms, *all_rooms;
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

  all_rooms = resolve_rooms[..];

  while(sizeof(resolve_rooms)) {
    res_tmp = 0;

    for(iter = 0; iter < sizeof(resolve_rooms);) {
      if(resolve_location(resolve_rooms[iter])) {
	res_tmp = 1;
	resolve_rooms = resolve_rooms[..iter-1] + resolve_rooms[iter+1..];
      } else {
	iter++;
      }
    }

    if(!res_tmp && sizeof(resolve_rooms)) {
      /* This isn't working, no new rooms are being resolved! */
      string tmp;
      int    ctr;

      tmp = "Can't resolve the following rooms: ";
      for(ctr = 0; ctr < sizeof(resolve_rooms) - 1; ctr++) {
	tmp += resolve_rooms[ctr]->get_number() + ", ";
      }
      tmp += resolve_rooms[sizeof(resolve_rooms) - 1]->get_number();
      LOGD->write_syslog(tmp);
      error("Can't resolve all rooms!  Edit room files to fix this!");
    }
  }

  for(iter = 0; iter < sizeof(all_rooms); iter++) {
    /* All rooms should now exist, so we can easily resolve
       pretty much anything else. */
    resolve_parent(all_rooms[iter]);
    resolve_removed_details(all_rooms[iter]);

    /* Clear all pending data */
    all_rooms[iter]->clear_pending();
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
