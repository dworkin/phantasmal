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
  zone_table = ({ ({ "Unzoned" }),
		    ({ "Miskatonic University" }),
		    ({ "Innsmouth" }),
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
