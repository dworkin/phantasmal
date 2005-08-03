/* This handler deals with TELOPTs that are fairly standard, but that
   DGD doesn't handle by default. */

/* So far, it handles the NAWS and TTYPE telnet options. */

#include "phantasmal/lpc_names.h"
#include "phantasmal/telnet.h"
#include "phantasmal/log.h"

#define TELOPT_TTYPE_SEND  1
#define TELOPT_TTYPE_IS    0

void telnet_do(int option) {
  switch(option) {
  case TELOPT_NAWS:
    previous_object()->send_telnet_option(TP_WONT, TELOPT_NAWS);
    break;
  case TELOPT_TTYPE:
    previous_object()->send_telnet_option(TP_WONT, TELOPT_TTYPE);
    break;
  }
}

void telnet_dont(int option) {
  switch(option) {
  case TELOPT_NAWS:
    previous_object()->send_telnet_option(TP_WONT, TELOPT_NAWS);
    break;
  case TELOPT_TTYPE:
    previous_object()->send_telnet_option(TP_WONT, TELOPT_TTYPE);
    break;
  }
}

void telnet_will(int option) {
  string tmp;

  switch(option) {
  case TELOPT_NAWS:
    /* Excellent.  We make no further requests, and the client should send
       us window-size info from here on out. */
    previous_object()->set_telopt(TELOPT_NAWS, 1);
    break;
  case TELOPT_TTYPE:
    previous_object()->set_telopt(TELOPT_TTYPE, 1);
    /* Have the client send out the first terminal type */
    tmp = " ";
    tmp[0] = TELOPT_TTYPE_SEND;
    previous_object()->send_telnet_subnegotiation(TELOPT_TTYPE, tmp);
    break;
  }
}

void telnet_wont(int option) {
  switch(option) {
  case TELOPT_NAWS:
    previous_object()->set_telopt(TELOPT_NAWS, 0);
    break;
  case TELOPT_TTYPE:
    previous_object()->set_telopt(TELOPT_TTYPE, 0);
    break;
  }
}

void telnet_sb(int option, string str) {
  int width, height;
  string new_term, tmp;

  switch(option) {
  case TELOPT_NAWS:
    if(strlen(str) != 4) {
      LOGD->write_syslog("Unexpected format for TELOPT_NAWS response!",
			 LOG_WARN);
      return;
    }
    width = str[0] * 256 + str[1];
    height = str[2] * 256 + str[3];

    LOGD->write_syslog("Setting window h/w to " + height + "," + width
		       + " from TELOPT_NAWS", LOG_VERBOSE);
    previous_object()->naws_window_size(width, height);
    break;
  case TELOPT_TTYPE:
    if(str[0] != TELOPT_TTYPE_IS) {
      LOGD->write_syslog("Received invalid TELOPT_TTYPE request!", LOG_WARN);
      return;
    }

    new_term = str[1..];
    if(!previous_object()->register_terminal_type(new_term)) {
      /* Have the client send out another terminal type if we just got
         a new one.  Collect 'em all! */
      tmp = " ";
      tmp[0] = TELOPT_TTYPE_SEND;
      previous_object()->send_telnet_subnegotiation(TELOPT_TTYPE, tmp);
    }
    break;
  }
}
