#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>
#include <type.h>
#include <limits.h>

/* ZoneD -- daemon that tracks zones */

inherit dtd DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);

mixed* zone_table;

/* Are we decoding the unq zonelist or segments */
int load_type;

static void create(varargs int clone) {
  if(clone)
    error("Cloning zoned is not allowed!");

  dtd::create(clone);

  if(!find_object(UNQ_DTD)) compile_object(UNQ_DTD);

  zone_table = ({ ({ "Unzoned", ([ ]) }) });

  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON()) {
    set_dtd_file(ZONED_DTD);
    dtd::upgraded(clone);
  }
}

void destructed(int clone) {
  if(SYSTEM()) {
    dtd::destructed(clone);
  }
}

/******* Init Function called by INITD *********************/

/* The zonefile's contents are passed through the string
   argument.  Currently zone files must be DGD's string
   size or less. */
void init_from_file(string file) {
  if(!SYSTEM())
    return;

  if(strlen(file) > MAX_STRING_SIZE - 3)
    error("Zonefile is too large in ZONED->init_from_file!");
  load_type = 1;
  from_unq_text(file);

  if(find_object(MAPD))
    MAPD->notify_new_zones();
}

/* Load in the available zones */
void init_zonelist_from_file(string file) {
  if(!SYSTEM())
    return;

  if(strlen(file) > MAX_STRING_SIZE - 3)
    error("Zonefile is too large in ZONED->init_from_file!");
  load_type = 2;
  from_unq_text(file);

  if(find_object(MAPD))
    MAPD->notify_new_zones();
}

/******* Functions for DTD_UNQABLE *************************/

mixed* to_dtd_unq(void) {
  int    ctr, highseg, zone, numzones;
  mixed *tmp, *zonetmp;

  if(!SYSTEM() && !COMMON())
    return nil;

  highseg = OBJNUMD->get_highest_segment();

  zonetmp = ({ "zonelist", ({ }) });
  numzones = sizeof(zone_table);
  for(ctr = 0; ctr < numzones; ctr++){
    zonetmp[1] +=  ({ ({ "zone",
		        ({ ({ "zonenum", ctr }),
  			   ({ "name", zone_table[ctr][0] }) 
		         })
	  	      })
	            });
  }
 
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

  return zonetmp + tmp;
}

string get_parse_error_stack(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  return ::get_parse_error_stack();
}

void from_dtd_unq(mixed* unq) {
  mixed *zones, *segment_unq;
  int segnum,zonenum;
  string zonename;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(sizeof(unq) != 4)
    error("There should be exactly one 'zones' and one 'zonelist'"
	  + " section in the ZONED file!");

  if(unq[0] != "zonelist")
    error("Unrecognized section in ZONED file -- must start with"
	  + " 'zonelist'!");
  else if(unq[2] != "zones")
    error("Unrecognized section in ZONED file -- second section"
	  + " must be 'zones'!");

  if (load_type == 1){

    zones = unq[3]; /* load zones values */
    while(sizeof(zones)) {
    
      /* Everything in the zones entry must be a segment. */
      if(typeof(zones[0]) != T_ARRAY
         || sizeof(zones[0]) < 2
         || zones[0][0] != "segment" )
        error("Format error in zone file, expected 'segment' section!");

      if( zones[0][0] == "segment" ){
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
      }
      zones = zones[1..];    
    }

  } else if( load_type == 2) {

    zones = unq[1]; /* load zonelist values */
    while(sizeof(zones)) {
      if( zones[0][0] == "zone" ){
          segment_unq = zones[0][1];
          zonenum = segment_unq[0][1];
          zonename = segment_unq[1][1];
          if (zonename != "Unzoned"){
            zone_table += ({ ({ zonename, ([ ]) }) });
            /* Is this zone in the position expected in the table? */
            if (zone_table[zonenum][0] != zonename){
              error("\nZONED: Incorrect zone table order "
		    + zonename + " #" + zonenum + "\n");
            }
          }
      } 
      /* Remove that segment, move on */
      zones = zones[1..];    
    }
    
  }

}

void write_to_file(string filename) {
  string str;
  string unq_str;

  if(previous_program() != "/usr/System/initd")
    error("Only INITD may instruct the ZONED to write files!");

  unq_str = dtd::to_unq_text();
  if(!unq_str)
    error("To_unq_text() returned nil!");
  if(!write_file(filename, unq_str)) {
    error("Error writing file!");
  }

}


/******* Regular ZONED functions ***************************/

int num_zones(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  return sizeof(zone_table);
}

string get_name_for_zone(int zonenum) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(zonenum >= 0 && sizeof(zone_table) > zonenum) {
    return zone_table[zonenum][0];
  } else {
    return nil;
  }
}

int* get_segments_in_zone(int zonenum) {
  mixed* keys;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

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

  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  roomnum = room->get_number();
  segment = roomnum / 100;
  zone = OBJNUMD->get_segment_zone(segment);

  return zone;
}

int add_new_zone( string zonename ){
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if (zonename && zonename != ""){
    int zonenum;
    zone_table += ({ ({ zonename, ([ ]) }) });

    /* And tell MAPD to update its zone table, too */
    MAPD->notify_new_zones();

    return num_zones()-1;
  } else {
    error("Illegal or (nil) zone name given to ZONED!");
  }
}
