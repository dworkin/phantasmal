#include <config.h>
#include <type.h>
#include <log.h>
#include <kernel/user.h>

inherit USER_STATE;

/* Vars for MAKEROOM user state */
private int    substate;

/* Data specified by user */
private int    room_number;
private string brief_desc;
private string glance_desc;
private string look_desc;
private string examine_desc;


/* Valid substate values */
#define SS_PROMPT_ROOM_NUMBER       1
#define SS_PROMPT_BRIEF_DESC        2
#define SS_PROMPT_GLANCE_DESC       3
#define SS_PROMPT_LOOK_DESC         4
#define SS_PROMPT_EXAMINE_DESC      5

/* Input function return values */
#define RET_NORMAL            1
#define RET_POP_STATE         2


/* Prototypes */
static int  prompt_room_number_input(string input);
static int  prompt_brief_desc_input(string input);
static int  prompt_glance_desc_input(string input);
static void prompt_look_desc_data(mixed data);
static void prompt_examine_desc_data(mixed data);


static void create(varargs int clone) {
  ::create();
  if(!find_object(US_ENTER_DATA)) compile_object(US_ENTER_DATA);
  if(clone) {
    substate = SS_PROMPT_ROOM_NUMBER;
  }
}

/* This handles input directly from the user.  Handling depends on the
   current substate of this user state. */
int from_user(string input) {
  int    ret;
  string quitcheck;

  if(input) {
    quitcheck = STRINGD->trim_whitespace(input);
    if(!STRINGD->stricmp(quitcheck, "quit")) {
      send_string("(Quitting OLC -- Cancel!)\r\n");
      pop_state();
      return MODE_ECHO;
    }
  }

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
  case SS_PROMPT_LOOK_DESC:
  case SS_PROMPT_EXAMINE_DESC:
    send_string("Internal error in state machine!  Cancelling OLC!\r\n");
    pop_state();
    return MODE_ECHO;
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

/* This is if somebody (other than us) is sending data to the
   user.  This happens if, for instance, somebody moves into
   or out of the same room and the player sees a message. */
void to_user(string output) {
  send_string(output);
}

/* This is called when the state is switched to.  The pushp parameter
   describes whether a push or a pop switched to this state -- if
   pushp is true, the state was just allocated and started.  If it's
   false, a state got pushed and we resumed control afterward. */
void switch_to(int pushp) {
  if(pushp && substate == SS_PROMPT_ROOM_NUMBER) {
    /* Just allocated */
    send_string("Creating a new room.  Type 'quit' at the prompt"
		+ " to cancel.\r\n");
    send_string("First, enter the desired room number,"
		+ " or hit enter to assign it automatically.\r\n");
    send_string(" > ");
  } else if (substate == SS_PROMPT_LOOK_DESC) {

  } else if (substate == SS_PROMPT_EXAMINE_DESC) {

  } else {
    /* Somebody else pushed and then popped a state, so we're just
       getting back to ourselves. */
    send_string("(Creating object -- resuming)\r\n");
    send_string(" > ");
  }
}

void switch_from(int popp) {
  if(!popp) {
    send_string("(Creating object -- suspending)\r\n");
  }
}

/* Some other state has passed us data, probably when it was
   popped. */
void pass_data(mixed data) {
  send_string("(data passed in)\r\n");

  switch(substate) {
  case SS_PROMPT_LOOK_DESC:
    prompt_look_desc_data(data);
    break;
  case SS_PROMPT_EXAMINE_DESC:
    prompt_examine_desc_data(data);
    break;
  default:
    send_string("Warning: User State was passed unrecognized data!\r\n");
    break;
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
  object edit_state;

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

  send_string("\r\nGlance desc accepted.\r\n");
  send_string("Now, enter a multiline 'look' description.  This is what an"
	      + " observer would note\r\n");
  send_string("specifically about this object on quick perusal.\r\n");

  edit_state = clone_object(US_ENTER_DATA);
  if(edit_state) {
    push_state(edit_state);
  } else {
    LOGD->write_syslog("Couldn't clone US_ENTER_DATA state object!",
		       LOG_ERROR);
    return RET_POP_STATE;
  }

  return RET_NORMAL;
}

static void prompt_look_desc_data(mixed data) {
  object edit_state;

  if(typeof(data) != T_STRING) {
    send_string("Non-string data passed to state!  Huh?  Cancelling.\r\n");
    pop_state();
    return;
  }

  look_desc = STRINGD->trim_whitespace(data);

  if(!data || STRINGD->is_whitespace(data)) {
    send_string("That look description was all whitespace.  "
		+ "Let's try that again.\r\n");

    edit_state = clone_object(US_ENTER_DATA);
    if(edit_state) {
      push_state(edit_state);
    } else {
      LOGD->write_syslog("Couldn't clone US_ENTER_DATA state object!",
			 LOG_ERROR);
      pop_state();
    }

    return;
  }

  substate = SS_PROMPT_EXAMINE_DESC;

  send_string("\r\nLook desc accepted.\r\n");
  send_string("Now, enter a multiline 'examine' description.  This is what an"
	      + " observer would\r\n");
  send_string("note about this object with careful scrutiny.\r\n");
  send_string("Or just hit enter and it will default to the look desc.\r\n");

  edit_state = clone_object(US_ENTER_DATA);
  if(edit_state) {
    push_state(edit_state);
  } else {
    LOGD->write_syslog("Couldn't clone US_ENTER_DATA state object!",
		       LOG_ERROR);
    pop_state();
    return;
  }

}

static void prompt_examine_desc_data(mixed data) {
  if(typeof(data) != T_STRING) {
    send_string("Non-string data passed to state!  Huh?  Cancelling.\r\n");
    pop_state();
    return;
  }

  if(!data || STRINGD->is_whitespace(data)) {
    send_string("Examine desc defaults to look desc.\r\n");
    examine_desc = nil;
  } else {
    examine_desc = STRINGD->trim_whitespace(data);
  }

  send_string("Test complete.\r\n");
  pop_state();
}
