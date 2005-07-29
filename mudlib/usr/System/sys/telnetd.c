#include <kernel/user.h>
#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <gameconfig.h>

static int    suspended, shutdown;

void upgraded(varargs int clone);

static void create(varargs int clone) {
  if(clone) {
    error("Can't clone TelnetD!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  if(!SYSTEM())
    return;

  /* The default user object isn't a System program any more, so
     any Common or System things that it'll need compiled should
     be compiled for it here.  Ditto for PHANTASMAL_USER. */
  if(!find_object(US_SCROLL_TEXT)) compile_object(US_SCROLL_TEXT);
  if(!find_object(SYSTEM_USER_OBJ)) compile_object(SYSTEM_USER_OBJ);

  if(!find_object(DEFAULT_USER_OBJ)) compile_object(DEFAULT_USER_OBJ);
}

void suspend_input(int shutdownp) {
  if(!SYSTEM() && !KERNEL())
    return;

  if(suspended)
    LOGD->write_syslog("Suspended again without release!", LOG_ERR);

  suspended = 1;
  if(shutdownp)
    shutdown = 1;
}

void release_input(void) {
  if(!SYSTEM() && !KERNEL())
    return;

  if(!suspended)
    LOGD->write_syslog("Released without suspend!", LOG_ERR);

  suspended = 0;
}

object select(string str)
{
  object game_driver;

  if(!SYSTEM() && !KERNEL())
    return nil;

  game_driver = CONFIGD->get_game_driver();

  if(game_driver)
    return game_driver->new_user_connection(str);

  return clone_object(DEFAULT_USER_OBJ);
}

int query_timeout(object connection)
{
  object game_driver;

  if(!SYSTEM() && !KERNEL())
    error("Invalid call to query_timeout!");

  game_driver = find_object(GAME_DRIVER);
  if(suspended || shutdown
     || (query_ip_number(connection) && game_driver
	 && game_driver->site_is_banned(query_ip_number(connection)))) {
    return -1;
  }

  return DEFAULT_TIMEOUT;
}

string query_banner(object connection)
{
  object game_driver;

  if(!SYSTEM() && !KERNEL())     return nil;

  game_driver = CONFIGD->get_game_driver();
  if(!game_driver) {
    if(shutdown)
      return "MUD is shutting down...  Try again later.\n";

    if(suspended)
      return "MUD is suspended.  Try again in a minute or two.\n";

    return "Phantasmal (no gamedriver)\n\nLogin: ";
  }

  if(shutdown)
    return game_driver->get_shutdown_message(connection);

  if(suspended)
    return game_driver->get_suspended_message(connection);

  if(query_ip_number(connection)
     && game_driver->site_is_banned(query_ip_number(connection))) {
    return game_driver->get_sitebanned_message(connection);
  }

  return game_driver->get_welcome_message(connection);
}
