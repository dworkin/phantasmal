#include <kernel/user.h>
#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>

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

  if(!find_object(DEFAULT_USER_OBJ)) { compile_object(DEFAULT_USER_OBJ); }
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
  if(!SYSTEM() && !KERNEL())
    return -1;

  if(suspended || shutdown)
    return -1;

  return DEFAULT_TIMEOUT;
}

string query_banner(object connection)
{
  object game_driver;

  if(!SYSTEM() && !KERNEL())
     return nil;

  game_driver = CONFIGD->get_game_driver();
  if(!game_driver) {
    if(shutdown)
      return "MUD is shutting down...  Try again later.\r\n";

    if(suspended)
      return "MUD is suspended.  Try again in a minute or two.\r\n";

    return "Phantasmal (no gamedriver)\r\n\r\nLogin: ";
  }

  if(shutdown)
    return game_driver->get_shutdown_message(connection);

  if(suspended)
    return game_driver->get_suspended_message(connection);

  return game_driver->get_welcome_message(connection);
}
