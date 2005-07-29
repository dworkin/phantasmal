#include <kernel/user.h>
#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/telnet.h>
#include <phantasmal/mudclient.h>

#include <gameconfig.h>

inherit COMMON_AUTO;

private int     suspended, shutdown;
private mapping telopt_handler;

int support_protocol;
mapping protocol_names;

void upgraded(varargs int clone);
string autodetect_client_str(void);

static void create(varargs int clone) {
  if(clone) {
    error("Can't clone MudClientD!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  object handler;

  if(!SYSTEM())
    return;

  /* The default user object isn't a System program any more, so
     any Common or System things that it'll need compiled should
     be compiled for it here.  Ditto for PHANTASMAL_USER. */
  if(!find_object(US_SCROLL_TEXT)) compile_object(US_SCROLL_TEXT);
  if(!find_object(MUDCLIENT_CONN)) compile_object(MUDCLIENT_CONN);

  support_protocol = 0;
  protocol_names = ([
		     "MCP" : PROTOCOL_MCP,
		     "IMP" : PROTOCOL_IMP,
		     "FIRECLIENT" : PROTOCOL_IMP,
		     "PUEBLO" : PROTOCOL_PUEBLO,
		     "MXP" : PROTOCOL_MXP,
		     "MSP" : PROTOCOL_MSP,
		     "ZMP" : PROTOCOL_ZMP,
		     "ZENITH" : PROTOCOL_ZMP,
		     "MCCP" : PROTOCOL_MCCP,
		     ]);
  if(!find_object(TELOPT_DEFAULT_HANDLER))
    compile_object(TELOPT_DEFAULT_HANDLER);
  handler = find_object(TELOPT_DEFAULT_HANDLER);

  telopt_handler = ([
		     TELOPT_ECHO : handler,
		     TELOPT_SGA : handler,
		     TELOPT_TM : handler,
		     TELOPT_LINEMODE : handler,
		     ]);
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
  object game_driver, conn;

  if(!SYSTEM() && !KERNEL())
    return nil;

  game_driver = CONFIGD->get_game_driver();

  conn = clone_object(MUDCLIENT_CONN);
  conn->receive_message(str);

  return conn;
}

int query_timeout(object connection)
{
  object game_driver;

  if(!SYSTEM() && !KERNEL())
    return -1;

  game_driver = find_object(GAME_DRIVER);
  if(suspended || shutdown
     || (query_ip_number(connection) && game_driver
	 && game_driver->site_is_banned(query_ip_number(connection)))) {
    return -1;
  }

  connection->set_mode(MODE_RAW);

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
      return "MUD is shutting down...  Try again later.\n\r";

    if(suspended)
      return "MUD is suspended.  Try again in a minute or two.\n\r";

    return "Phantasmal (no gamedriver)\n\r\n\rLogin: ";
  }

  telnet_options = "";

  if(shutdown)
    send_back = game_driver->get_shutdown_message(connection);
  else if(suspended)
    send_back = game_driver->get_suspended_message(connection);
  else if(query_ip_number(connection)
	  && game_driver->site_is_banned(query_ip_number(connection)))
    return game_driver->get_sitebanned_message(connection);
  else {
    string proto_options;

    /* Okay, so if all of that doesn't happen...  Then we should
       probably negotiate a proper telnet connection, not just send a
       message telling them to bugger off and then close the connection.

       It's a shame, really.  I *like* closing the connection on them.
    */

    send_back = game_driver->get_welcome_message(connection);
    if(!send_back)
      error("(nil) welcome message on Mudclient port!");

    send_back = autodetect_client_str() + send_back;

    /* Start with IAC WONT TELOPT_ECHO, IAC DO TELOPT_LINEMODE */
    telnet_options = "      ";
    telnet_options[0] = TP_IAC;
    telnet_options[1] = TP_WONT;
    telnet_options[2] = TELOPT_ECHO;
    telnet_options[3] = TP_IAC;
    telnet_options[4] = TP_DO;
    telnet_options[5] = TELOPT_LINEMODE;

    proto_options = "   ";
    proto_options[0] = TP_IAC;
    proto_options[1] = TP_WILL;

    if(support_protocol & PROTOCOL_MXP) {
      proto_options[2] = TELOPT_MXP;
      telnet_options = telnet_options + proto_options;
    }
    if(support_protocol & PROTOCOL_MSP) {
      proto_options[2] = TELOPT_MSP;
      telnet_options = telnet_options + proto_options;
    }
    if(support_protocol & PROTOCOL_ZMP) {
      proto_options[2] = TELOPT_ZMP;
      telnet_options = telnet_options + proto_options;
    }
  }

  send_back = implode(explode(send_back, "\r"), "");
  send_back = implode(explode(send_back, "\n"), "\n\r");

  return telnet_options + send_back;
}

void protocol_allow(string protocol, int should_attempt) {
  int pnum;

  protocol = STRINGD->to_upper(protocol);

  if(protocol_names[protocol]) {
    pnum = protocol_names[protocol];
  } else {
    error("Unrecognized protocol " + protocol + " requested in MUDCLIENTD!");
  }

  if(should_attempt)
    support_protocol |= pnum;
  else
    support_protocol &= ~pnum;
}

string autodetect_client_str(void) {
  string ret;

  ret = "";
  if(support_protocol & PROTOCOL_IMP)
    ret += "Autodetecting IMP...v1.30\n\r";

  if(support_protocol & PROTOCOL_MCP)
    ret += "#$#mcp version: 2.1 to: 2.1\n\r";

  if(support_protocol & PROTOCOL_PUEBLO)
    ret += "This world is Pueblo 2.50 enhanced.\n\r";

  return ret;
}

void set_telopt_handler(int option_num, object handler) {
  if(SYSTEM() || COMMON() || GAME()) {
    if(option_num == TELOPT_SGA || option_num == TELOPT_ECHO
       || option_num == TELOPT_TM || option_num == TELOPT_LINEMODE) {
      LOGD->write_syslog("Setting uncommon handler for standard telnet"
			 + "option!", LOG_WARN);
    }

    telopt_handler[option_num] = handler;
  } else {
    error("Non-privileged code attempted to call "
	  + "MUDCLIENTD::set_telopt_handler!");
  }
}

object get_telopt_handler(int option_num) {
  return telopt_handler[option_num];
}
