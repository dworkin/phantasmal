#include <config.h>
#include <type.h>
#include <kernel/kernel.h>
#include <log.h>
#include <limits.h>

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

/******* Init Function called by INITD *********************/

/* The zonefile's contents are passed through the string
   argument.  Currently zone files must be DGD's string
   size or less. */
void init_from_file(string file) {
  if(strlen(file) > MAX_STRING_SIZE - 3)
    error("Zonefile is too large in ZONED->init_from_file!");

  from_unq_text(file);
}


/******* Functions for DTD_UNQABLE *************************/

mixed* to_dtd_unq(void) {
  int    ctr, highseg, zone;
  mixed *tmp;

  highseg = OBJNUMD->get_highest_segment();
  tmp = ({ "zones", ({ }) });
  for(ctr = 0; ctr <= highseg; ctr++) {
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

string get_parse_error_stack(void) {
  return ::get_parse_error_stack();
}

void from_dtd_unq(mixed* unq) {
  mixed *zones, *segment_unq;

  if(sizeof(unq) > 2)
    error("There should be only one zones section in the ZONED file!");

  if(unq[0] != "zones")
    error("Unrecognized section in ZONED file -- must start with 'zones'!");

  zones = unq[1];
  while(sizeof(zones)) {
    int segnum, zonenum;

    /* Everything in the zones entry must be a segment. */
    if(typeof(zones[0]) != T_ARRAY
       || sizeof(zones[0]) < 2
       || zones[0][0] != "segment")
      error("Format error in zone file, expected 'segment' section!");

    segment_unq = zones[0][1];

    if(sizeof(segment_unq) != 2
       || segment_unq[0][0] != "segnum"
       || segment_unq[1][0] != "zonenum") {
      error("ZONED segment doesn't fit format "
	    + "[segnum, <int>, zonenum, <int>]!");
    }

    segnum = segment_unq[0][1];
    zonenum = segment_unq[1][1];

    /* Set zone for segment */
    if(OBJNUMD->get_segment_owner(segnum)) {
      OBJNUMD->set_segment_zone(segnum, zonenum);
    } else {
      LOGD->write_syslog("Unowned segment, dropping seg #" + segnum,
			 LOG_WARN);
    }

    /* Remove that segment, move on */
    zones = zones[1..];
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

  if(sizeof(zone_table) <= zonenum) {
    error("Can't add segment to nonexistent zone!");
  }
  zone_table[zonenum][1][segment] = 1;
}

void remove_segment_from_zone(int zonenum, int segment) {
  if(previous_program() != OBJNUMD)
    error("Only OBJNUMD can remove segments from a zone!");

  if(zonenum < 0)
    return;

  if(sizeof(zone_table) <= zonenum) {
    LOGD->write_syslog("Nonexistent zone, not in table!", LOG_WARN);
    return;
  }

  zone_table[zonenum][1][segment] = nil;
}

int get_zone_for_room(object room) {
  int roomnum, segment, zone;

  roomnum = room->get_number();
  segment = roomnum / 100;
  zone = OBJNUMD->get_segment_zone(segment);

  return zone;
}
