#include <config.h>
#include <type.h>
#include <kernel/kernel.h>
#include <log.h>

/* PortableD -- daemon that tracks standard MUD carryable objects, including
   collection types for same. */

private int*    portable_segments;
private mixed*  collections;

private object  port_dtd;


/* Prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  if(clone)
    error("Cloning portabled is not allowed!");

  portable_segments = ({ });
  collections = ({ });

  if(!find_object(UNQ_DTD)) compile_object(UNQ_DTD);
  if(!find_object(SIMPLE_PORTABLE)) compile_object(SIMPLE_PORTABLE);

  upgraded();
}

void upgraded(varargs int clone) {
  string dtd_file;

  dtd_file = read_file(PORTABLE_DTD);
  if(!dtd_file)
    error("Can't read Portable DTD file " + PORTABLE_DTD + "!");

  if(!port_dtd) {
    port_dtd = clone_object(UNQ_DTD);
  }
  port_dtd->clear();
  port_dtd->load(dtd_file);
}

void destructed(int clone) {
  if(port_dtd)
    destruct_object(port_dtd);
}

int* get_portable_segments(void) {
  return portable_segments[..];
}

int* portables_in_segment(int segment) {
  if(sizeof( ({ segment }) & portable_segments )) {
    return OBJNUMD->objects_in_segment(segment);
  }

  return nil;
}

object get_portable_by_num(int num) {
  int seg;

  seg = num / 100;
  if(sizeof(({ seg }) & portable_segments)) {
    return OBJNUMD->get_object(num);
  }

  return nil;
}

private int assign_portable_number(int num, object port) {
  int    segnum, ctr;
  string segown;

  if(!port)
    error("Assigning nil portable!");

  if(num != -1) {
    segnum = num / 100;

    segown = OBJNUMD->get_segment_owner(segnum);
    if(segown && strlen(segown) && segown != PORTABLED) {
      LOGD->write_syslog("Can't allocate number " + num
			 + " in somebody else's segment!", LOG_WARN);
      return -1;
    }

    OBJNUMD->allocate_in_segment(segnum, num, port);
    if(!sizeof(portable_segments & ({ segnum }))) {
      portable_segments += ({ segnum });
    }
    return num;
  } else {
    for(ctr = 0; ctr < sizeof(portable_segments); ctr++) {
      num = OBJNUMD->new_in_segment(portable_segments[ctr], port);
      if(num != -1)
        break;
    }
    if(num == -1) {
      segnum = OBJNUMD->allocate_new_segment();
      portable_segments += ({ segnum });
      num = OBJNUMD->new_in_segment(segnum, port);
    }

    return num;
  }

}

void add_portable_number(object portable, int num) {
  int    seg;

  if(!portable)
    error("Nil passed to add_portable_number!");

  num = assign_portable_number(num, portable);
  if(num < 0) {
    error("Can't allocate portable number!");
  }

  portable->set_number(num);
}


private int resolve_parent(object port) {
  int pending;
  object parent;

  pending = port->get_pending_parent();

  parent = MAPD->get_room_by_num(pending);
  if(!parent) {
    parent = PORTABLED->get_portable_by_num(pending);
  }
  if(!parent) {
    return 0;
  }

  port->set_archetype(parent);
  return 1;
}


/* Filename will be used later to register dependency with the objectd
   and/or manually do reloads in the PortableD. */
void add_dtd_unq_portables(mixed* unq_data, object dflt_location,
			   string filename) {
  object port, location;
  int    iter, pending;
  mixed* resolve_loc, *resolve_par;

  resolve_loc = ({ });
  resolve_par = ({ });
  iter = 0;
  while(iter < sizeof(unq_data)) {
    port = clone_object(SIMPLE_PORTABLE);
    if(!port) {
      error("Can't clone new portable!");
    }
    port->from_dtd_unq( ({ unq_data[iter], unq_data[iter + 1] }) );

    pending = port->get_pending_location();
    if(pending == -1) {
      if(!dflt_location) {
	error("Can't load locationless portables:  no default placement!");
      }
      dflt_location->add_to_container(port);
    } else {
      resolve_loc += ({ port });
    }

    pending = port->get_pending_parent();
    if(pending != -1) {
      resolve_par += ({ port });
    }

    PORTABLED->add_portable_number(port, port->get_number());

    iter += 2;
  }

  /* Now go through and resolve location of unresolved portables */
  while(sizeof(resolve_loc)) {
    int size, ctr, pending;

    size = sizeof(resolve_loc);

    ctr = 0;
    while(ctr < sizeof(resolve_loc)) {
      pending = resolve_loc[ctr]->get_pending_location();

      location = MAPD->get_room_by_num(pending);
      if(!location) {
	location = PORTABLED->get_portable_by_num(pending);
      }

      if(location) {
	location->add_to_container(resolve_loc[ctr]);
	resolve_loc = resolve_loc[..ctr-1] + resolve_loc[ctr+1..];
      } else {
	ctr++;
      }
    }

    if(size == sizeof(resolve_loc)) {
      LOGD->write_syslog("Can't find locations for "
			 + size + " portables -- leaving them hanging!");
      return;
    }
  }

  /* Go through and resolve parents of unresolved portables */
  for(iter = 0; iter < sizeof(resolve_par); iter++) {
    if(!resolve_parent(resolve_par[iter])) {
      error("Can't find parent number (#"
	    + resolve_par[iter]->get_pending_parent()
	    + ") loading portable!");
    }
  }
}


void add_unq_text_portables(string text, object dflt_location,
			    string filename) {
  mixed* unq_data;

  unq_data = UNQ_PARSER->unq_parse_with_dtd(text, port_dtd);
  if(!unq_data)
    error("Cannot parse text in add_unq_text_portables!");

  add_dtd_unq_portables(unq_data, dflt_location, filename);
}
