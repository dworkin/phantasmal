/* This is a TELOPT handler to emulate default DGD handling of telnet
   options.  It also includes some other good default option-handling
   behavior.  You can specifically use it to handle appropriate
   subsets of TELOPT options. */

#include "phantasmal/telnet.h"

void telnet_do(int option) {
  switch(option) {
  case TELOPT_TM:
    previous_object()->send_telnet_option(TP_WONT, TELOPT_TM);
    break;
  case TELOPT_SGA:
    previous_object()->should_suppress_ga(1);
    previous_object()->send_telnet_option(TP_WILL, TELOPT_SGA);
    break;
  }
}

void telnet_dont(int option) {
  switch(option) {
  case TELOPT_SGA:
    previous_object()->should_suppress_ga(0);
    previous_object()->send_telnet_option(TP_WONT, TELOPT_SGA);
    break;
  }
}

void telnet_will(int option) {
  string tmp;

  switch(option) {
  case TELOPT_LINEMODE:
    tmp = "  ";
    tmp[0] = LM_MODE;
    tmp[1] = MODE_EDIT;
    previous_object()->send_telnet_subnegotiation(TELOPT_LINEMODE, tmp);
    break;
  }
}

void telnet_wont(int option) {

}

void telnet_sb(int option, string str) {
}
