#include <kernel/user.h>
#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/telnet.h>

static int    suspended, shutdown;

void upgraded(varargs int clone);

static void create(varargs int clone) {
  if(clone) {
    error("Can't clone MudClientD!");
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
  if(!SYSTEM() && !KERNEL())
    return -1;

  if(suspended || shutdown)
    return -1;

  return DEFAULT_TIMEOUT;
}

string query_banner(object connection)
{
  object game_driver;
  string send_back, telnet_options;

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

  /* Okay, so if all of that doesn't happen...  Then we should
     probably negotiate a proper telnet connection, not just send a
     message telling them to bugger off and then close the connection.

     It's a shame, really.  I *like* closing the connection on them.
  */

  send_back = game_driver->get_welcome_message(connection);
  if(!send_back) return send_back;

  /* Return IAC WONT TELOPT_ECHO, IAC DO TELOPT_LINEMODE */
  telnet_options = "      ";
  telnet_options[0] = TP_IAC;
  telnet_options[1] = TP_WONT;
  telnet_options[2] = TELOPT_ECHO;
  telnet_options[3] = TP_IAC;
  telnet_options[4] = TP_DO;
  telnet_options[5] = TELOPT_LINEMODE;
  return telnet_options + send_back;
}
