#include <config.h>
#include <kernel/user.h>

inherit USER_STATE;

/* Vars for MAKEROOM user state */
private int    substate;

/* Data specified by user */
private int    room_number;
private string brief_desc;
private string glance_desc;
private string look_desc;


/* Valid substate values */
#define SS_PROMPT_ROOM_NUMBER       1
#define SS_PROMPT_BRIEF_DESC        2
#define SS_PROMPT_GLANCE_DESC       3
#define SS_PROMPT_LOOK_DESC         4

/* Input function return values */
#define RET_NORMAL            1
#define RET_POP_STATE         2


/* Prototypes */
static int prompt_room_number_input(string input);
static int prompt_brief_desc_input(string input);
static int prompt_glance_desc_input(string input);


static void create(varargs int clone) {
  ::create();
  if(clone) {
    substate = SS_PROMPT_ROOM_NUMBER;
  }
}

int from_user(string input) {
  int ret;

  switch(substate) {
  case SS_PROMPT_ROOM_NUMBER:
    ret = prompt_room_number_input(input);
    break;
  case SS_PROMPT_BRIEF_DESC:
    ret = prompt_brief_desc_input(input);
    break;
  case SS_PROMPT_GLANCE_DESC:
    ret = prompt_glance_desc_input(input);
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
  /* send_string("\r\n(Suspending OLC for output...)\r\n"); */
  send_string(output);
  /* send_string("\r\n(Re-enabling OLC after output!)\r\n"); */
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
  if(!input || STRINGD->is_whitespace(input)) {
    /* Autoassign */
    room_number = -1;

    send_string("Room number will be assigned automatically.\r\n");
  } else {
    if(sscanf(input, "%*s %*d") == 2
       || sscanf(input, "%*d %*s") == 2
       || sscanf(input, "%d", room_number) != 1) {
      send_string("Please *only* enter a number.  Enter a room number"
		  + " or hit enter.\r\n");
      return RET_NORMAL;
    }
    /* Room number was parsed. */
    if(room_number < 1) {
      send_string("That doesn't appear to be a legal room number.\r\n");
      send_string("Enter a (positive, nonzero) room number or hit enter.\r\n");

      return RET_NORMAL;
    }

    /* Okay, room number looks good -- continue. */
  }

  send_string("Next, please enter a one-line brief description.\r\n");
  send_string("Examples of brief descriptions:  "
	      + "'a sword', 'John', 'some bacon'.\r\n");

  substate = SS_PROMPT_BRIEF_DESC;

  return RET_NORMAL;
}

static int prompt_brief_desc_input(string input) {
  if(!input || STRINGD->is_whitespace(input)) {
    send_string("That was all whitespace.  Let's try that again.\r\n");
    send_string("Please enter a one-line brief description.\r\n");
    send_string("Examples of brief descriptions:  "
		+ "'a sword', 'John', 'some bacon'.\r\n");

    return RET_NORMAL;
  }

  brief_desc = STRINGD->trim_whitespace(input);
  substate = SS_PROMPT_GLANCE_DESC;

  send_string("Please enter a one-line glance description.\r\n");
  send_string("Examples of brief descriptions:  "
	      + "'a red flashing toy gun', 'John the Butcher',"
	      + "or 'about a pound of bacon'.\r\n");

  return RET_NORMAL;
}

static int prompt_glance_desc_input(string input) {
  if(!input || STRINGD->is_whitespace(input)) {
    send_string("That was all whitespace.  Let's try that again.\r\n");
    send_string("Please enter a one-line glance description.\r\n");
    send_string("Examples of brief descriptions:  "
		+ "'a red flashing toy gun', 'John the Butcher',"
		+ "or 'about a pound of bacon'.\r\n");

    return RET_NORMAL;
  }

  glance_desc = STRINGD->trim_whitespace(input);
  substate = SS_PROMPT_LOOK_DESC;

  send_string("Now, enter a multiline 'look' description.  This is what an"
	      + " observer would note\r\n");
  send_string("specifically about this object on quick perusal.\r\n");

  send_string("Successful input test!  Switching back.\r\n");

  return RET_POP_STATE;
}
