#include <config.h>

private mapping segments;
private string* owners;
private int     segments_full;
private int     highest_segment;

/* Prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  if(clone) {
    error("Can't clone objnumd!");
  }
  segments = ([ ]);
  segments_full = -1;
  highest_segment = -1;

  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON())
    owners = ({ MAPD, EXITD, MOBILED });
}

void destructed(varargs int clone) {
  if(SYSTEM()) {

  }
}

string get_segment_owner(int segment) {
  if(!SYSTEM() && !COMMON())
    return nil;

  if(segments[segment]) {
    return owners[segments[segment][0]];
  } else
    return nil;
}

int get_highest_segment(void) {
  if(!SYSTEM() && !COMMON())
    return -1;

  return highest_segment;
}

private void set_segment_owner(int segment, int owner, int zonenum) {
  if(segment < 0)
    error("Can't allocate negative segment in set_segment_owner!");

  if(segments[segment]) {
    segments[segment][0] = owner;
    return;
  }

  /* This location defines segment structure... */
  segments[segment] = ({ owner, ({ }), zonenum });

  if(segments_full == segment - 1) {
    while(segments[segments_full + 1])
      segments_full++;
  }
  if(segment > highest_segment)
    highest_segment = segment;
}

int get_segment_zone(int segment) {
  mixed* seg;

  if(!SYSTEM() && !COMMON())
    return -1;

  seg = segments[segment];
  if(seg) {
    return seg[2];
  }

  /* unzoned */
  return 0;
}

void set_segment_zone(int segment, int zonenum, varargs int req_own) {
  mixed* seg;
  int    owner, oldzone;

  if(previous_program() == SYSTEM_WIZTOOLLIB
     || previous_program() == ZONED) {

    /* The wiztoollib and the ZoneD both have full access to the
       setting of zones.  That's because wiztoollib holds the
       admin commands to alter zone numbers and ZoneD does the
       tracking. */
    owner = -1;
    if(req_own)
      owner = req_own;
  } else {
    for(owner = 0; owner < sizeof(owners); owner++) {
      if(previous_program() == owners[owner])
	break;
    }
    if(owner >= sizeof(owners)
       || owners[owner] != previous_program())
      error("Unknown owner " + previous_program()
	    + " calling set_segment_zone!");
  }
  if(owner != -1 && req_own)
    error("A normal segment owner may not request to impersonate another!");

  seg = segments[segment];
  if(!seg) {
    if(owner == -1) {
      error("Can't set_segment_zone on a nonexistent, unowned segment!");
    }
    set_segment_owner(segment, owner, zonenum);
    ZONED->add_segment_to_zone(zonenum, segment);
    return;
  }
  if(owner != -1 && seg[0] != owner)
    error("Can't set zone of somebody else's segment!");

  oldzone = seg[2];
  ZONED->remove_segment_from_zone(oldzone, segment);
  seg[2] = zonenum;
  ZONED->add_segment_to_zone(zonenum, segment);

  /* Notify the segment owner */
  call_other(owners[segments[segment][0]], "set_segment_zone", segment,
	     zonenum, oldzone);
}

int allocate_new_segment(void) {
  int owner;
  int seg;

  for(owner = 0; owner < sizeof(owners); owner++) {
    if(previous_program() == owners[owner])
      break;
  }
  if(owner >= sizeof(owners)
     || owners[owner] != previous_program())
    error("Unknown owner " + previous_program()
	  + " calling allocate_new_segment!");

  seg = segments_full + 1;
  if(get_segment_owner(seg)) {
    error("Internal error -- attempting to reassign segment!");
  }
  /* Set as unzoned (zone #0) */
  set_segment_owner(seg, owner, 0);

  return seg;
}

private void unallocate_segment(int segment) {
  error("Should not use?");

  if(segments_full >= segment) {
    segments_full = segment - 1;
  }

  if(segment == highest_segment) {
    error("Haven't implemented recalculating highest segment!");
  }

  segments[segment] = nil;
}

/* Note:  doing an allocate_in_segment with a valid tracking number
   and nil as the object is a quite acceptable way to allocate or
   pre-grow a segment */
void allocate_in_segment(int segment, int tr_num, object obj) {
  int    offs;
  mixed* seg;
  int    owner;

  if(tr_num < 0)
    error("Negative tracking number in allocate_in_segment!");
  if(tr_num / 100 != segment)
    error("Tracking number not in segment in allocate_in_segment!");

  for(owner = 0; owner < sizeof(owners); owner++) {
    if(previous_program() == owners[owner])
      break;
  }
  if(owner >= sizeof(owners)
     || owners[owner] != previous_program())
    error("Unknown owner " + previous_program()
	  + " calling allocate_in_segment!");

  offs = tr_num % 100;
  seg = segments[segment];
  if(!seg) {
    /* Allocate a new segment for caller, unzoned */
    set_segment_owner(segment, owner, 0);
    seg = segments[segment];
    if(!seg)
      error("Cannot allocate segment -- why?");
  }
  if(seg[0] != owner)
    error("Can't allocate in somebody else's segment!");

  if(sizeof(segments[segment][1]) <= offs) {
    seg[1] += allocate(offs - sizeof(seg[1]) + 1);
  }

  if(seg[1][offs])
    error("Reassigning used number #" + tr_num + " in allocate_in_segment!");

  seg[1][offs] = obj;
}

/* Note: because a destructed object's references will all become
   nil automatically, doing a remove_from_segment is unnecessary
   if the object has already been destructed */
void remove_from_segment(int segment, int tr_num) {
  mixed* seg;
  int    offs;

  seg = segments[segment];
  if(!seg)
    error("Can't remove from unallocated segment!");

  if(tr_num / 100 != segment)
    error("Can't remove tr_num from different segment!");

  offs = tr_num % 100;

  if(sizeof(seg[1]) <= offs || !seg[1][offs])
    error("Object not in segment in remove_from_segment!");

  seg[1][offs] = nil;
}

/* A segment owner calls this function to retrieve an object from its
   own segment. */
object get_object(int tr_num) {
  int    segment, offs;
  int    owner;
  object ret;

  if(tr_num < 0)
    error("Tracking numbers must be >= 0!");
  segment = tr_num / 100;
  offs = tr_num % 100;
  owner = segments[segment] ? segments[segment][0] : -1;
  if(owner == -1
     || owners[owner] != previous_program()) {
    return nil;
  }

  if(sizeof(segments[segment][1]) <= offs) {
    return nil;
  }

  ret = segments[segment][1][offs];

  return ret;
}

/* Attempts to allocate a new tracking number in the given
   segment.  If the segment is full, it returns -1. */
int new_in_segment(int segment, object obj) {
  mixed* seg;
  int    ctr;

  seg = segments[segment];
  if(owners[seg[0]] != previous_program())
    error("Can't allocate in segment you don't own!");

  for(ctr = 0; ctr < sizeof(seg[1]); ctr++) {
    if(!seg[1][ctr]) {
      seg[1][ctr] = obj;
      return segment * 100 + ctr;
    }
  }

  if(sizeof(seg[1]) < 100) {
    int offs;

    offs = sizeof(seg[1]);

    seg[1] += allocate(1);
    seg[1][offs] = obj;

    return segment * 100 + offs;
  }

  return -1;
}

/* Returns a list of object numbers in an owned segment.  Caller
   must be the segment owner and the segment must be allocated.
*/
int* objects_in_segment(int segment) {
  int*   objs;
  int    ctr, tr_num;
  mixed* seg;

  seg = segments[segment];
  if(!seg || previous_program() != owners[seg[0]])
    error("Can't get listing of a segment you don't own!");

  objs = ({ });
  for(ctr = 0, tr_num = segment*100; ctr < sizeof(seg[1]); ctr++, tr_num++) {
    if(seg[1][ctr]) {
      objs += ({ tr_num });
    }
  }

  return objs;
}
