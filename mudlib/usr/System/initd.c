#include <kernel/kernel.h>
#include <kernel/access.h>
#include <kernel/rsrc.h>
#include <kernel/version.h>
#include <type.h>
#include <config.h>
#include <log.h>

inherit access API_ACCESS;
inherit rsrc   API_RSRC;

static void create(varargs int clone)
{
  object driver, obj, the_void;
  string mapd_dtd, help_dtd, objs_file;
  int major, minor, patch;

  /* First things first -- this release needs one of the
     latest versions of DGD, so let's make sure. */
  if(!sscanf(KERNEL_LIB_VERSION, "%d.%d.%d", major, minor, patch)) {
    error("Don't recognize Kernel Library version as being of the"
	  + " form 1.2.XX!");
  }
  if((major == 1 && minor < 2)
     || (major == 1 && minor == 2 && patch < 13)) {
    error("Need to upgrade to DGD version 1.2.41 or higher!");
  } else if (major > 1 || (major == 1 && minor > 2)) {
    DRIVER->message("This version of Phantasmal is not tested");
    DRIVER->message("with DGD beyond 1.2.XX.  Please upgrade!\n");
  }

  access::create();
  rsrc::create();

  add_user("common");
  add_owner("common");
  set_global_access("common", READ_ACCESS);

  driver = find_object(DRIVER);

  /* Start LOGD and log MUD startup */
  if(!find_object(LOGD)) { compile_object(LOGD); }
  LOGD->write_syslog("\n-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-\n"
		     + "Starting MUD...\n"
		     + "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-");

  /* Compile, find and install the Errord */
  if(!find_object(ERRORD)) { compile_object(ERRORD); }
  driver->set_error_manager(find_object(ERRORD));

  /* Compile, find and install the Objectd */
  if(!find_object(OBJECTD)) { compile_object(OBJECTD); }
  driver->set_object_manager(find_object(OBJECTD));
  OBJECTD->do_initial_obj_setup();

  /* Compile the String manager (used as a std library) */
  if(!find_object(STRINGD)) { compile_object(STRINGD); }

  /* Start up logging channels in the LOGD */
  LOGD->start_channels();

  /* Compile, find and install the Telnetd */
  if(!find_object(TELNETD)) { compile_object(TELNETD); }
  "/kernel/sys/userd"->set_telnet_manager(0,find_object(TELNETD));
  /* "/kernel/sys/userd"->set_telnet_manager(find_object(TELNETD)); */

  /* Compile the Phrase manager (before Helpd) */
  if(!find_object(PHRASED)) { compile_object(PHRASED); }

  /* Set up online help */
  if(!find_object(HELPD)) { compile_object(HELPD); }

  help_dtd = read_file(HELP_DTD);
  if(!help_dtd)
    error("Can't load file " + HELP_DTD + "!");

  HELPD->load_help_dtd(help_dtd);
  HELPD->new_help_directory("/data/help");

  /* Compile the Objnumd */
  if(!find_object(OBJNUMD)) { compile_object(OBJNUMD); }

  /* Set up Mapd & Exitd */
  if(!find_object(MAPD)) { compile_object(MAPD); }
  if(!find_object(EXITD)) { compile_object(EXITD); }

  mapd_dtd = read_file(MAPD_ROOM_DTD);
  if(!mapd_dtd)
    error("Can't read file " + MAPD_ROOM_DTD + "!");
  MAPD->init(mapd_dtd);

  /* Set up The Void (room #0) with "start room" alias */
  if(!find_object(THE_VOID)) { compile_object(THE_VOID); }
  the_void = clone_object(THE_VOID);
  if(!the_void)
    error("Can't clone void object!");
  MAPD->add_room_number(the_void, 0);
  MAPD->set_room_alias("start room", the_void);

  /* Set up the PortableD */
  if(!find_object(PORTABLED)) { compile_object(PORTABLED); }

  /* Load stuff into MAPD and EXITD */
  objs_file = read_file(ROOM_FILE);
  if(!objs_file)
    error("Can't read file " + ROOM_FILE + "!");
  MAPD->add_unq_text_rooms(objs_file, ROOM_FILE);
  EXITD->add_deferred_exits();

  /* Load stuff into PORTABLED */
  objs_file = read_file(PORT_FILE);
  if(!objs_file)
    error("Can't read file " + PORT_FILE + "!");
  PORTABLED->add_unq_text_portables(objs_file, nil, PORT_FILE);

  if(!find_object(CHANNELD)) compile_object(CHANNELD);
  
}

private void save_mud_data(void) {
  int*   objects;
  object obj;
  int    ctr;
  string unq_str;

  remove_file(ROOM_FILE + ".old");
  rename_file(ROOM_FILE, ROOM_FILE + ".old");
  remove_file(ROOM_FILE);

  remove_file(PORT_FILE + ".old");
  rename_file(PORT_FILE, PORT_FILE + ".old");
  remove_file(PORT_FILE);

  if(sizeof(get_dir(ROOM_FILE)[0])) {
    LOGD->write_syslog("Can't remove old roomfile -- trying to append!",
		       LOG_FATAL);
  }

  if(sizeof(get_dir(PORT_FILE)[0])) {
    LOGD->write_syslog("Can't remove old portablefile -- trying to append!",
		       LOG_FATAL);
  }

  

  LOGD->write_syslog("Writing rooms to file", LOG_NORMAL);
  objects = MAPD->rooms_in_zone(0) - ({ 0 });
  for(ctr = 0; ctr < sizeof(objects); ctr++) {
    obj = MAPD->get_room_by_num(objects[ctr]);

    unq_str = obj->to_unq_text();

    if(!write_file(ROOM_FILE, unq_str)) {
      DRIVER->message("Couldn't write rooms to file!  Fix or kill driver!");
      error("Couldn't write rooms to file " + ROOM_FILE + "!");
    }
  }

  LOGD->write_syslog("Writing portables to file", LOG_NORMAL);

  /* Assign a list of all portables to "objects" */
  {
    int* tmp, *tmp2;

    objects = ({ });
    tmp = PORTABLED->get_portable_segments();
    for(ctr = 0; ctr < sizeof(tmp); ctr++) {
      tmp2 = PORTABLED->portables_in_segment(tmp[ctr]);
      if(tmp2)
	objects += tmp2;
    }
  }

  for(ctr = 0; ctr < sizeof(objects); ctr++) {
    obj = PORTABLED->get_portable_by_num(objects[ctr]);

    unq_str = obj->to_unq_text();

    if(!write_file(PORT_FILE, unq_str)) {
      DRIVER
	->message("Couldn't write portables to file!  Fix or kill driver!");
      error("Couldn't write portables to file " + PORT_FILE + "!");
    }
  }

}

void prepare_reboot(void)
{
  if(previous_program() != SYSTEM_WIZTOOLLIB
     && previous_program() != DRIVER)
    error("Can't call prepare_reboot from there!");

  if(find_object(LOGD)) {
    LOGD->write_syslog("Preparing to reboot MUD...");
  }

  save_mud_data();
}

void prepare_shutdown(void)
{
  if(previous_program() != SYSTEM_WIZTOOLLIB
     && previous_program() != DRIVER)
    error("Can't call prepare_shutdown from there!");

  if(find_object(LOGD)) {
    LOGD->write_syslog("Shutting down MUD...");
  }

  save_mud_data();
}

void reboot(void)
{
  if(previous_program() != SYSTEM_WIZTOOLLIB)
    error("Can't call reboot from there!");

  if(find_object(LOGD)) {
    LOGD->write_syslog("Rebooting!");
  }
}
