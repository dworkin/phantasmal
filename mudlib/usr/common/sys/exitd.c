#include <config.h>
#include <type.h>
#include <kernel/kernel.h>
#include <log.h>

#include <map.h>

inherit PHRASE_REPOSITORY;

private int*    exit_segments;
private mapping name_for_dir;
private mapping shortname_for_dir;
private mapping builder_directions;

/* When loading files, this lets us resolve room numbers *after* all rooms
   load... */
private mixed*  deferred_add_exit;

/* Prototypes */
void add_simple_exit_between(object room1, object room2, int direction,
			     int num1, int num2);
void upgraded(varargs int clone);
#define PHR(x) PHRASED->new_simple_english_phrase(x)
#define FILE(x) PHRASED->file_phrase(EXITD_PHRASES,(x))

static void create(varargs int clone) {
  if(clone)
    error("Cloning exitd is not allowed!");

  ::create(clone);

  if(!find_object(SIMPLE_EXIT))
    compile_object(SIMPLE_EXIT);

  exit_segments = ({ });

  deferred_add_exit = ({ });

  upgraded();

  if(!load_filemanaged_file(EXITD_PHRASES)) {
    error("Can't load ExitD phrase file!");
  }
}

void upgraded(varargs int clone) {
  ::upgraded();

  name_for_dir = ([ DIR_NORTH : FILE("north"),
		    DIR_SOUTH : FILE("south"),
		    DIR_EAST : FILE("east"),
		    DIR_WEST : FILE("west"),
		    DIR_NORTHWEST : FILE("northwest"),
		    DIR_NORTHEAST : FILE("northeast"),
		    DIR_SOUTHWEST : FILE("southwest"),
		    DIR_SOUTHEAST : FILE("southeast"),
		    DIR_IN : FILE("in"),
		    DIR_OUT : FILE("out"),
		    DIR_UP : FILE("up"),
		    DIR_DOWN : FILE("down"),
		    ]);

  shortname_for_dir = ([ DIR_NORTH : FILE("n"),
			 DIR_SOUTH : FILE("s"),
			 DIR_EAST : FILE("e"),
			 DIR_WEST : FILE("w"),
			 DIR_NORTHWEST : FILE("nw"),
			 DIR_NORTHEAST : FILE("ne"),
			 DIR_SOUTHWEST : FILE("sw"),
			 DIR_SOUTHEAST : FILE("se"),
			 DIR_IN : FILE("i"),
			 DIR_OUT : FILE("o"),
			 DIR_UP : FILE("u"),
			 DIR_DOWN : FILE("d"),
			 ]);

  /* This is the set of direction string acceptable for
     builder commands (currently) and file formats (probably forever) */
  builder_directions = ([ "north"      : DIR_NORTH,
			  "south"      : DIR_SOUTH,
			  "east"       : DIR_EAST,
			  "west"       : DIR_WEST,
			  "northeast"  : DIR_NORTHEAST,
			  "northwest"  : DIR_NORTHWEST,
			  "southeast"  : DIR_SOUTHEAST,
			  "southwest"  : DIR_SOUTHWEST,
			  "up"         : DIR_UP,
			  "down"       : DIR_DOWN,
			  "in"         : DIR_IN,
			  "out"        : DIR_OUT,

			  "n"  : DIR_NORTH,
			  "s"  : DIR_SOUTH,
			  "e"  : DIR_EAST,
			  "w"  : DIR_WEST,
			  "ne" : DIR_NORTHEAST,
			  "nw" : DIR_NORTHWEST,
			  "se" : DIR_SOUTHEAST,
			  "sw" : DIR_SOUTHWEST,
			  "u"  : DIR_UP,
			  "d"  : DIR_DOWN,
			  ]);
}

void destructed(int clone) {

}


/* Return phrase for direction name */
object get_name_for_dir(int direction) {
  return name_for_dir[direction];
}

/* Return phrase for direction short name */
object get_short_for_dir(int direction) {
  object phr;
  phr = shortname_for_dir[direction];
  if(!phr) error("Can't get short name for direction " + direction);
  return phr;
}

int direction_by_string(string direc) {
  if(builder_directions[direc])
    return builder_directions[direc];

  return -1;
}


int opposite_direction(int direction) {
  /* The map.h table of directions has a cute feature... */
  if(direction % 2) {
    return direction + 1;
  }

  return direction - 1;
}

private void push_or_add_exit(int roomnum1, int roomnum2, int direction,
			      int num1, int num2) {
  object room1, room2;

  room1 = MAPD->get_room_by_num(roomnum1);
  room2 = MAPD->get_room_by_num(roomnum2);

  if(room1 && room2) {
    add_simple_exit_between(room1, room2, direction, num1, num2);
  } else {
    deferred_add_exit += ({ ({ roomnum1, roomnum2, direction, num1, num2 }) });
  }
}

void room_request_simple_exit(int roomnum1, int roomnum2, int direction,
			      int num1, int num2) {
  if(previous_program() != SIMPLE_ROOM) {
    error("Only SIMPLE_ROOM can request deferred exit creation!");
  }

  push_or_add_exit(roomnum1, roomnum2, direction, num1, num2);
}

void add_deferred_exits(void) {
  int    ctr;
  mixed* exits, *ex;

  exits = deferred_add_exit;
  deferred_add_exit = ({ });

  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    ex = exits[ctr];
    push_or_add_exit(ex[0], ex[1], ex[2], ex[3], ex[4]);
  }

}

int num_deferred_exits(void) {
  if(!deferred_add_exit) return -1;

  return sizeof(deferred_add_exit);
}

private int allocate_exit_obj(int num, object obj) {
  int segment;

  if(num >= 0 && OBJNUMD->get_object(num))
    error("Object already exists with number " + num);

  if(num != -1) {
    if(!(sizeof( ({ num / 100 }) & exit_segments ))) {
      string tmp;

      exit_segments |= ({ num / 100 });
      tmp = "Exit segments: ";
      for(segment = 0; segment < sizeof(exit_segments); segment++) {
	tmp += exit_segments[segment] + " ";
      }
      LOGD->write_syslog(tmp, LOG_NORMAL);
    }

    OBJNUMD->allocate_in_segment(num / 100, num, obj);
    return num;
  }

  for(segment = 0; segment < sizeof(exit_segments); segment++) {
    num = OBJNUMD->new_in_segment(exit_segments[segment], obj);
    if(num != -1) {
      return num;
    }
  }

  segment = OBJNUMD->allocate_new_segment();

  exit_segments += ({ segment });
  num = OBJNUMD->new_in_segment(segment, obj);

  return num;
}

void add_simple_exit_between(object room1, object room2, int direction,
			     int num1, int num2) {
  object exit1, exit2;

  exit1 = clone_object(SIMPLE_EXIT);
  exit2 = clone_object(SIMPLE_EXIT);

  exit1->set_destination(room2);
  exit1->set_direction(direction);

  exit2->set_destination(room1);
  exit2->set_direction(opposite_direction(direction));

  if(exit1->get_destination() != room2)
    error("Error assigning dest to exit!");

  if(exit2->get_destination() != room1)
    error("Error assigning dest to exit!");

  if(exit1->get_direction() != direction) {
    error("Exit has incorrect dir "
	  + STRINGD->mixed_sprint(exit1->get_direction()));
    error("Error assigning dir to exit!");
  }

  if(exit2->get_direction() != opposite_direction(direction))
    error("Error assigning dir to exit!");

  room1->add_exit(direction, exit1);
  room2->add_exit(opposite_direction(direction), exit2);

  num1 = allocate_exit_obj(num1, exit1);
  num2 = allocate_exit_obj(num2, exit2);

  if(num1 < 0 || num2 < 0) {
    error("Exit numbers not assigned successfully!");
  }

  exit1->set_number(num1);
  exit2->set_number(num2);

  if(exit1->get_number() < 0
     || exit2->get_number() < 0)
    error("Exit numbers not assigned successfully!");

  exit1->set_brief(PHRASED->new_simple_english_phrase("Exit #" + num1));
  exit2->set_brief(PHRASED->new_simple_english_phrase("Exit #" + num2));
}

void remove_exit(object room, object exit) {
  int iter, opp_dir;
  object other_exit, dest;

  dest = exit->get_destination();

  if(dest) {
    opp_dir = EXITD->opposite_direction(exit->get_direction());
    other_exit = dest->get_exit(opp_dir);
    if(other_exit) {
      if(other_exit->get_destination() == room) {
	exit->get_destination()->remove_exit(other_exit);
	destruct_object(other_exit);
      } else {
	LOGD->write_syslog("Opposite exit doesn't go same place!",
			   LOG_WARNING);
      }
    } else {
      LOGD->write_syslog("No opposite exit to " + exit->get_number()
			 + "!", LOG_ERROR);
    }
  } else {
    LOGD->write_syslog("Destination should always be non-nil right now!",
		       LOG_ERROR);
  }

  room->remove_exit(exit);
  destruct_object(exit);
}

void clear_all_exits(object room) {
  object exit;

  if(!room)
    error("Passed nil to clear_all_exits!");

  while(exit = room->get_exit_num(0)) {
    remove_exit(room, exit);
  }

  room->clear_exits();
}

object get_exit_by_num(int num) {
  int seg;

  if(num < 0) return nil;
  seg = num / 100;
  if(sizeof( ({ seg }) & exit_segments )) {
    return OBJNUMD->get_object(num);
  }

  return nil;
}

int* get_exit_segments(void) {
  return exit_segments[..];
}

int* get_all_exits(void) {
  int* exits, *tmp;
  int  iter;

  exits = ({ });
  for(iter = 0; iter < sizeof(exit_segments); iter++) {
    tmp = OBJNUMD->objects_in_segment(exit_segments[iter]);
    if(tmp)
      exits += tmp;
  }

  if(!sizeof(exits))
    return nil;

  return exits;
}

int* exits_in_segment(int seg) {
  if(sizeof( ({ seg}) & exit_segments )) {
    return OBJNUMD->objects_in_segment(seg);
  }

  return nil;
}
