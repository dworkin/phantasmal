#include <config.h>
#include <kernel/user.h>

inherit USER_STATE;

/* Vars for MAKEROOM user state */
private int    substate;
private object user;

/* Valid substate values */
#define SS_PROMPT_ROOM_NUMBER       1

/* Input function return values */
#define RET_NORMAL            1
#define RET_POP_STATE         2

static int prompt_room_number_input(string input);

static void create(varargs int clone) {
  ::create();
  if(clone) {
    substate = SS_PROMPT_ROOM_NUMBER;
  }
}

/* This sets variables supplied by the caller (wiztool, usually) */
static void init(object new_user) {
  user = new_user;
}

int from_user(string input) {
  int ret;

  switch(substate) {
  case SS_PROMPT_ROOM_NUMBER:
    ret = prompt_room_number_input(input);
    break;
  default:
    send_string("Unrecognized state!  Cancelling OLC!\r\n");
    pop_state();
    return MODE_ECHO;
  }

  switch(ret) {
  case RET_NORMAL:
    send_string(" > ");
    break;
  case RET_POP_STATE:
    pop_state();
    break;
  default:
    send_string("Unrecognized return value!  Cancelling OLC!\r\n");
    pop_state();
    break;
  }

  return MODE_ECHO;
}

void to_user(string output) {
  send_string("\r\n(Suspending OLC for output...)\r\n");
  send_string(output);
  send_string("\r\n(Re-enabling OLC after output!)\r\n");
}

void switch_to(int pushp) {
  if(pushp && substate == SS_PROMPT_ROOM_NUMBER) {
    send_string("Creating a new room.  Type 'quit' at any time"
		+ " to cancel.\r\n");
    send_string("First, enter the desired room number,"
		+ " or hit enter to assign it automatically.\r\n");
    send_string(" > ");
  } else {
    send_string("(Creating object -- resuming)\r\n");
  }
}

void switch_from(int popp) {
  if(!popp) {
    send_string("(Creating object -- suspending)\r\n");
  }
}

static int prompt_room_number_input(string input) {
  send_string("Successful input test!  Switching back.");
  return RET_POP_STATE;
}
