#include <config.h>
#include <type.h>
#include <kernel/kernel.h>
#include <log.h>

/* ZoneD -- daemon that tracks zones -- currently mostly hardcoded */

inherit dtd DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);

mixed* zone_table;

static void create(varargs int clone) {
  if(clone)
    error("Cloning zoned is not allowed!");

  dtd::create(clone);

  if(!find_object(UNQ_DTD)) compile_object(UNQ_DTD);

  upgraded();
}

void upgraded(varargs int clone) {
  set_dtd_file(ZONED_DTD);
  dtd::upgraded(clone);
  zone_table = ({ ({ "Unzoned", ([ ]) }),
		    ({ "Miskatonic University", ([ ]) }),
		    ({ "Innsmouth", ([ ]) }),
		    });
}

void destructed(int clone) {
  dtd::destructed(clone);
}

/******* Functions for DTD_UNQABLE *************************/

mixed* to_dtd_unq(void) {
  int    ctr, highseg, zone;
  mixed *tmp;

  highseg = OBJNUMD->get_highest_segment();
  tmp = ({ "zones", ({ }) });
  for(ctr = 0; ctr < highseg; ctr++) {
    if(!OBJNUMD->get_segment_owner(ctr))
      continue;

    zone = OBJNUMD->get_segment_zone(ctr);
    tmp[1] += ({ ({ "segment",
		      ({ ({ "segnum", ctr }),
			   ({ "zonenum", zone })
			   })
		      })
		   });
  }

  return tmp;
}

void from_dtd_unq(mixed* unq) {
  mixed *zones, *segment;

  if(sizeof(unq) > 2)
    error("There should be only one zones section in the ZONED file!");

  if(unq[0] != "zones")
    error("Unrecognized section in ZONED file -- must start with 'zones'!");

  zones = unq[1];
  while(sizeof(zones)) {
    /* Everything in the zones entry must be a segment. */
    segment = zones[1];

    /* Remove that segment, move on */
    zones = zones[2..];

  }
}

void write_to_file(string filename) {
  if(previous_program() != "/usr/System/initd")
    error("Only INITD may instruct the ZONED to write files!");

  LOGD->write_syslog("Writing to file " + filename);
  dtd::write_to_file(filename);
}


/******* Regular ZONED functions ***************************/

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
