#include <kernel/kernel.h>
#include <config.h>
#include <type.h>
#include <log.h>

/* MobileD-owned Segments */
private int*    mobile_segments;

/* Whether the MOBILED has been initialized */
private int     initialized;

/* The mapping of tag names to files */
private mapping tag_code;

private object  mobfile_dtd;
private object  binder_dtd;

/* Prototypes */
        void   upgraded(varargs int clone);
private int    allocate_mobile_obj(int num, object obj);
private object add_struct_for_mobile(mixed* unq);
        void   add_dtd_unq_mobiles(mixed *unq, string filename);

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  if(clone)
    error("Cloning mobiled is not allowed!");

  mobile_segments = ({ });

  upgraded();

}

private void load_tag_codes(void) {
  string bind_file, tag, file;
  int    ctr;
  mixed  unq_data;

  bind_file = read_file(MOBILE_BIND_FILE);
  if(!bind_file)
    error("Cannot read binder file " + MOBILE_BIND_FILE + "!");

  unq_data = UNQ_PARSER->unq_parse_with_dtd(bind_file, binder_dtd);
  if(!unq_data)
    error("Cannot parse binder text in MOBILED::init()!");

  if (sizeof(unq_data) % 2)
    error("Odd sized unq chunk in MOBILED::init()!");

  for (ctr = 0; ctr < sizeof(unq_data); ctr += 2) {
    if (STRINGD->stricmp(unq_data[ctr],"bind"))
      error("Not a code/tag binding in MOBILED::init()!");

    if (!STRINGD->stricmp(unq_data[ctr+1][0][0],"tag")) {
      tag = unq_data[ctr+1][0][1];
      file = unq_data[ctr+1][1][1];
    } else {
      tag = unq_data[ctr+1][1][1];
      file = unq_data[ctr+1][0][1];
    }

    if (tag_code[tag] != nil) {
      error("Tag " + tag + " is already bound in MOBILED::init()!");
    }

    tag_code[tag] = file;
    if(!find_object(file))
      compile_object(file);
  }
}

void upgraded(varargs int clone) {
  if(!SYSTEM() && previous_program() != MOBILED)
    return;

  /* Reload the Binder */
  tag_code = ([ ]);

  if(mobfile_dtd && binder_dtd) {
    load_tag_codes();
  }
}

void destructed(int clone) {
  if(SYSTEM()) {
    if(mobfile_dtd)
      destruct_object(mobfile_dtd);
    if(binder_dtd)
      destruct_object(mobfile_dtd);
  }
}

void init(void) {
  string mobfile_dtd_string, binder_dtd_string;

  if(!SYSTEM())
    return;

  if(!initialized) {
    mobfile_dtd_string = read_file(MOB_FILE_DTD);
    if(!mobfile_dtd_string)
      error("Can't read file " + MOB_FILE_DTD + "!");

    binder_dtd_string = read_file(BIND_DTD);
    if(!binder_dtd_string)
      error("Can't read file " + BIND_DTD + "!");

    binder_dtd = clone_object(UNQ_DTD);
    binder_dtd->load(binder_dtd_string);

    mobfile_dtd = clone_object(UNQ_DTD);
    mobfile_dtd->load(mobfile_dtd_string);

    load_tag_codes();
  } else
    error("MOBILED is already initialized in MOBILED::init()!");

  initialized = 1;
}


int add_mobile_number(object mobile, int num) {
  int newnum;

  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(!mobile)
    error("No mobile supplied to MOBILED::add_mobile_number!");

  if(!initialized)
    error("Can't add mobiles to uninitialized MOBILED!");

  newnum = allocate_mobile_obj(num, mobile);
  if(newnum <= 0) {
    error("Can't allocate mobile number!");
  }

  LOGD->write_syslog("Allocating mobile number: " + newnum, LOG_VERBOSE);

  mobile->set_number(newnum);

  return newnum;
}

private int allocate_mobile_obj(int num, object obj) {
  int segment;

  if(num >= 0 && OBJNUMD->get_object(num))
    error("Object already exists with number " + num);

  if(num != -1) {
    OBJNUMD->allocate_in_segment(num / 100, num, obj);

    if(!(sizeof( ({ num / 100 }) & mobile_segments ))) {
      string tmp;

      mobile_segments |= ({ num / 100 });
    }

    return num;
  }

  for(segment = 0; segment < sizeof(mobile_segments); segment++) {
    num = OBJNUMD->new_in_segment(mobile_segments[segment], obj);
    if(num != -1) {
      return num;
    }
  }

  segment = OBJNUMD->allocate_new_segment();
  LOGD->write_syslog("Allocating segment " + segment + " to MOBILED.",
		     LOG_VERBOSE);

  mobile_segments += ({ segment });
  num = OBJNUMD->new_in_segment(segment, obj);

  return num;
}

/* This function needs to remove the mobile from the list of mobiles
   in the mobile's containing room.  Since that list is not directly
   exported or modifiable, mobiles must currently be destroyed by
   removing their body from a room, destructing the mobile, then
   adding the object back. */
void remove_mobile(object mobile) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(mobile) {
    object body, location;

    body = mobile->get_body();
    if(body) {
      location = body->get_location();
      if(location) {
	location->remove_from_container(body);
	destruct_object(mobile);
	location->add_to_container(body);
      }
    }

    if(mobile)
      destruct_object(mobile);
  } else {
    error("Can't usefully remove a (nil) mobile!");
  }
}

object get_mobile_by_num(int num) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(num < 0) return nil;

  return OBJNUMD->get_object(num);
}

int* mobiles_in_segment(int seg) {
  int* tmp;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  tmp = OBJNUMD->objects_in_segment(seg);
  if(!tmp)
    tmp = ({ });

  return tmp;
}

int* all_mobiles(void) {
  int  iter;
  int* ret, *tmp;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  ret = ({ });
  for(iter = 0; iter < sizeof(mobile_segments); iter++) {
    tmp = OBJNUMD->objects_in_segment(mobile_segments[iter]);
    if(tmp)
      ret += tmp;
  }

  return ret;
}


string get_file_by_mobile_type(string mobtype) {
  if(!SYSTEM())
    error("Only SYSTEM code can query the MOBILED for mobile types!");

  return tag_code[mobtype];
}

object clone_mobile_by_type(string mobtype) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  if(!tag_code[mobtype]) return nil;

  return clone_object(tag_code[mobtype]);
}

void add_unq_text_mobiles(string unq_text, string filename) {
  mixed *unq_data;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(!initialized)
    error("Can't add mobiles to uninitialized MOBILED!");

  unq_data = UNQ_PARSER->unq_parse_with_dtd(unq_text, mobfile_dtd);
  if(!unq_data) {
    if(filename) { 
      error("Cannot parse file '" + filename
	    + "' in add_unq_text_mobiles!");
   } else {
      error("Cannot parse text in add_unq_text_mobiles!");
    }
  }

  add_dtd_unq_mobiles(unq_data, filename);
}

void add_dtd_unq_mobiles(mixed *unq, string filename) {
  int    iter;
  object mobile;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(!initialized)
    error("Can't add mobiles to uninitialized MOBILED!");

  iter = 0;
  while(iter < sizeof(unq)) {
    mobile = add_struct_for_mobile( ({ unq[iter], unq [iter + 1] }) );
    if(!mobile)
      error("Loaded mobile is (nil) in add_dtd_unq_mobiles!");
    iter += 2;
  }
}

private object add_struct_for_mobile(mixed* unq) {
  object mobile, body;
  int    num;
  mixed* ctr;
  string type;

  /* no unq passed in, so no mobile passed out */
  if (!unq || sizeof(unq) == 0) {
    return nil;
  }

  if(STRINGD->stricmp(unq[0], "mobile")) {
    error("UNQ doesn't look like a mobile in add_struct_for_mobile!");
  }

  ctr = unq[1];
  while(sizeof(ctr)) {
    if(!STRINGD->stricmp(ctr[0][0],"type")) {
      type = ctr[0][1];
      break;
    }
    ctr = ctr[1..];
  }
  if(!type) {
    /* There doesn't seem to be a "type" field in the UNQ passed in */
    error("Can't find 'type' field in mobfile!  "
	  + "Do you need to delete or update it?");
  }

  if(!tag_code[type]) {
    error("Can't find binding for mobile type '" + type + "'.");
  }

  mobile = clone_object(tag_code[type]);
  mobile->from_dtd_unq(unq[1]);

  num = mobile->get_number();
  num = add_mobile_number(mobile, num);
  if(num < 0) {
    error("Can't assign mobile number in add_struct_for_mobile!");
  }
  mobile->set_number(num);

  body = mobile->get_body();
  if(body) {
    body->set_mobile(mobile);
  } else {
    LOGD->write_syslog("Body is (nil), mob #" + num, LOG_WARN);
  }

  LOGD->write_syslog("Added struct for mobile!", LOG_VERBOSE);

  return mobile;
}
