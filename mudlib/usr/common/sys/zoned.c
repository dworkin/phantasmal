#include <config.h>
#include <type.h>
#include <kernel/kernel.h>
#include <log.h>

/* ZoneD -- daemon that tracks zones -- currently mostly hardcoded */

/* Prototypes */
void upgraded(varargs int clone);

mixed* zone_table;

static void create(varargs int clone) {
  if(clone)
    error("Cloning zoned is not allowed!");

  if(!find_object(UNQ_DTD)) compile_object(UNQ_DTD);

  upgraded();
}

void upgraded(varargs int clone) {
  zone_table = ({ ({ "Unzoned", ([ ]) }),
		    ({ "Miskatonic University", ([ ]) }),
		    ({ "Innsmouth", ([ ]) }),
		    });
}

void destructed(int clone) {

}

int num_zones(void) {
  return sizeof(zone_table);
}

string get_name_for_zone(int zonenum) {
  if(zonenum >= 0 && sizeof(zone_table) > zonenum) {
    return zone_table[zonenum][0];
  } else {
    return nil;
  }
}

mixed* get_segments_in_zone(int zonenum) {
  mixed* keys;

  keys = map_indices(zone_table[zonenum][1]);
  return keys;
}

void add_segment_to_zone(int zonenum, int segment) {
  if(previous_program() != OBJNUMD)
    error("Only OBJNUMD can add segments to a zone!");
  zone_table[zonenum][1][segment] = 1;
}

void remove_segment_from_zone(int zonenum, int segment) {
  if(previous_program() != OBJNUMD)
    error("Only OBJNUMD can remove segments from a zone!");
  zone_table[zonenum][1][segment] = nil;
}

int get_zone_for_room(object room) {
  int roomnum, segment, zone;

  roomnum = room->get_number();
  segment = roomnum / 100;
  zone = OBJNUMD->get_segment_zone(segment);

  return zone;
}
