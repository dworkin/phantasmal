#include <config.h>
#include <type.h>
#include <log.h>
#include <kernel/user.h>

inherit USER_STATE;

/* Vars for MAKEROOM user state */
private int    substate;

private object new_obj;

/* Data specified by user */
private int    obj_number;
private int    obj_type;


/* Valid object-type values */
#define OT_UNKNOWN                  1
#define OT_ROOM                     2
#define OT_PORTABLE                 3

/* Valid substate values */
#define SS_PROMPT_OBJ_TYPE          1
#define SS_PROMPT_OBJ_NUMBER        2
#define SS_PROMPT_BRIEF_DESC        3
#define SS_PROMPT_GLANCE_DESC       4
#define SS_PROMPT_LOOK_DESC         5
#define SS_PROMPT_EXAMINE_DESC      6
#define SS_PROMPT_NOUNS             7
#define SS_PROMPT_ADJECTIVES        8
#define SS_PROMPT_CONTAINER         9
#define SS_PROMPT_OPEN             10
#define SS_PROMPT_OPENABLE         11

/* Input function return values */
#define RET_NORMAL            1
#define RET_POP_STATE         2
#define RET_NO_PROMPT         3


/* Prototypes */
static int  prompt_obj_type_input(string input);
static int  prompt_obj_number_input(string input);
static int  prompt_brief_desc_input(string input);
static int  prompt_glance_desc_input(string input);
static void prompt_look_desc_data(mixed data);
static void prompt_examine_desc_data(mixed data);
static int  prompt_nouns_input(string input);
static int  prompt_adjectives_input(string input);
static void prompt_container_data(mixed data);
static void prompt_open_data(mixed data);
static void prompt_openable_data(mixed data);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)


static void create(varargs int clone) {
  ::create();
  if(!find_object(US_ENTER_DATA)) compile_object(US_ENTER_DATA);
  if(!find_object(US_ENTER_YN)) compile_object(US_ENTER_YN);
  if(!find_object(LWO_PHRASE)) compile_object(LWO_PHRASE);
  if(!find_object(SIMPLE_ROOM)) compile_object(SIMPLE_ROOM);
  if(!find_object(SIMPLE_PORTABLE)) compile_object(SIMPLE_PORTABLE);
  if(clone) {
    substate = SS_PROMPT_OBJ_TYPE;
    obj_type = OT_UNKNOWN;
    obj_number = -1;
  }
}

void specify_type(string type) {
  if(type == "room" || type == "r")
    obj_type = OT_ROOM;
  else if(type == "port" || type == "portable" || type == "p")
    obj_type = OT_PORTABLE;
  else
    error("Illegal value supplied to specify_type!");

  substate = SS_PROMPT_OBJ_NUMBER;
}

/* This handles input directly from the user.  Handling depends on the
   current substate of this user state. */
int from_user(string input) {
  int    ret;
  string quitcheck;

  if(input) {
    quitcheck = STRINGD->trim_whitespace(input);
    if(!STRINGD->stricmp(quitcheck, "quit")) {
      if(new_obj) {
	send_string("(Quitting OLC -- leaving obj #"
		    + new_obj->get_number() + " -- Cancel!)\r\n");
      } else {
	send_string("(Quitting OLC -- Cancel!)\r\n");
      }
      pop_state();
      return MODE_ECHO;
    }
  }

  switch(substate) {
  case SS_PROMPT_OBJ_TYPE:
    ret = prompt_obj_type_input(input);
    break;
  case SS_PROMPT_OBJ_NUMBER:
    ret = prompt_obj_number_input(input);
    break;
  case SS_PROMPT_BRIEF_DESC:
    ret = prompt_brief_desc_input(input);
    break;
  case SS_PROMPT_GLANCE_DESC:
    ret = prompt_glance_desc_input(input);
    break;
  case SS_PROMPT_NOUNS:
    ret = prompt_nouns_input(input);
    break;
  case SS_PROMPT_ADJECTIVES:
    ret = prompt_adjectives_input(input);
    break;

  case SS_PROMPT_LOOK_DESC:
  case SS_PROMPT_EXAMINE_DESC:
  case SS_PROMPT_CONTAINER:
  case SS_PROMPT_OPEN:
  case SS_PROMPT_OPENABLE:
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
  case RET_NO_PROMPT:
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
  /* For the moment, suspend output to the player who is mid-OLC */
  /* send_string(output); */
}

/* This is called when the state is switched to.  The pushp parameter
   describes whether a push or a pop switched to this state -- if
   pushp is true, the state was just allocated and started.  If it's
   false, a state got pushed and we resumed control afterward. */
void switch_to(int pushp) {
  if(pushp && substate == SS_PROMPT_OBJ_NUMBER) {
    /* Just allocated */
    send_string("Creating a new object.  Type 'quit' at the prompt"
		+ " (except on multiline prompts) to cancel.\r\n");
    send_string("First, enter the desired object number,"
		+ " or hit enter to assign it automatically.\r\n");
    send_string(" > ");
  } else if(pushp && substate == SS_PROMPT_OBJ_TYPE) {
    /* Just allocated */
    send_string("Creating a new object.  Type 'quit' at most prompts"
		+ " (but not multiline prompts) to cancel.\r\n");
    send_string("Please enter an object type.\r\n");
    send_string("Valid values are:  room, portable (r/p)\r\n");
    send_string(" > ");
  } else if (substate == SS_PROMPT_LOOK_DESC
	     || substate == SS_PROMPT_EXAMINE_DESC) {
    /* Do nothing */
  } else if (substate == SS_PROMPT_NOUNS) {
    /* This means we just got back from getting the Examine desc */
    send_string(" > ");
  } else {
    /* Somebody else pushed and then popped a state, so we're just
       getting back to ourselves. */
    send_string("(Creating object -- resuming)\r\n");
    send_string(" > ");
  }
}

void switch_from(int popp) {
  if(!popp) {
    if(substate != SS_PROMPT_LOOK_DESC
       && substate != SS_PROMPT_EXAMINE_DESC
       && substate != SS_PROMPT_CONTAINER
       && substate != SS_PROMPT_OPEN
       && substate != SS_PROMPT_OPENABLE) {
      send_string("(Creating object -- suspending)\r\n");
    }
  }
}

/* Some other state has passed us data, probably when it was
   popped. */
void pass_data(mixed data) {
  switch(substate) {
  case SS_PROMPT_LOOK_DESC:
    prompt_look_desc_data(data);
    break;
  case SS_PROMPT_EXAMINE_DESC:
    prompt_examine_desc_data(data);
    break;
  case SS_PROMPT_CONTAINER:
    prompt_container_data(data);
    break;
  case SS_PROMPT_OPEN:
    prompt_open_data(data);
    break;
  case SS_PROMPT_OPENABLE:
    prompt_openable_data(data);
    break;
  default:
    send_string("Warning: User State was passed unrecognized data!\r\n");
    break;
  }
}

static int prompt_obj_type_input(string input) {
  if(input)
    input = STRINGD->trim_whitespace(STRINGD->to_lower(input));

  /* TODO:  we should use the binder for this */
  if(!input
     || (input != "r" && input != "p" && input != "room" && input != "port"
	 && input != "portable")) {
    send_string("Valid values for object type are 'room' and 'portable'.\r\n");
    send_string("Please enter object type or 'quit'.\r\n");

    return RET_NORMAL;
  }

  if(input[0] == "r"[0]) {
    obj_type = OT_ROOM;
  } else {
    obj_type = OT_PORTABLE;
  }

  send_string("Type the desired object number,"
	      + " or hit enter to assign it automatically.\r\n");

  substate = SS_PROMPT_OBJ_NUMBER;

  /* The editor is going to print its own prompt, so don't bother
     with ours. */
  return RET_NORMAL;
}

static int prompt_obj_number_input(string input) {
  string segown;
  object location;
  int    zonenum;

  if(!input || STRINGD->is_whitespace(input)) {
    /* Autoassign */
    obj_number = -1;

    send_string("Object number will be assigned automatically.\r\n");
  } else {
    if(sscanf(input, "%*s %*d") == 2
       || sscanf(input, "%*d %*s") == 2
       || sscanf(input, "%d", obj_number) != 1) {
      send_string("Please *only* enter a number.  Enter an object number"
		  + " or hit enter.\r\n");
      return RET_NORMAL;
    }
    /* Object number was parsed. */
    if(obj_number < 1) {
      send_string("That doesn't appear to be a legal object number.\r\n");
      send_string("Enter a positive, nonzero object number or hit enter.\r\n");

      return RET_NORMAL;
    }
    if(MAPD->get_room_by_num(obj_number)) {
      send_string("There is already an object with that number.\r\n");
      send_string("Enter an object number, type 'quit' or hit enter.\r\n");

      return RET_NORMAL;
    }
    segown = OBJNUMD->get_segment_owner(obj_number / 100);
    if(obj_number >= 0 && segown && segown != MAPD) {
      user->message("Object number " + obj_number
		    + " is in a segment somebody else is using!\r\n");
      send_string("Enter an object number, type 'quit' or hit enter.\r\n");
      return RET_NORMAL;
    }

    /* Okay, object number looks good -- continue. */
  }

  location = get_user()->get_location();
  if(location && obj_type == OT_ROOM) {
    /* The new room should be put into the same place as the room
       the user is currently standing in.  Makes a good default. */
    location = location->get_location();
  }

  if(obj_type == OT_ROOM) {
    new_obj = clone_object(SIMPLE_ROOM);
  } else {
    new_obj = clone_object(SIMPLE_PORTABLE);
  }
  zonenum = -1;
  if(obj_number < 0) {
    if(get_user()->get_location()) {
      zonenum = ZONED->get_zone_for_room(get_user()->get_location());
    } else {
      zonenum = 0;
    }
    if(zonenum < 0) {
      LOGD->write_syslog("Odd, zone is less than 0 in @make_room...",
			 LOG_WARN);
      zonenum = 0;
    }
  }
  MAPD->add_room_to_zone(new_obj, obj_number, zonenum);

  zonenum = ZONED->get_zone_for_room(new_obj);

  if(location) {
    location->add_to_container(new_obj);
  }

  send_string("Added obj #" + new_obj->get_number()
	      + " to zone #" + zonenum
	      + " (" + ZONED->get_name_for_zone(zonenum) + ")" + ".\r\n");
  send_string("Its location is #" + location->get_number()
	      + "(" + location->get_brief()->to_string(get_user())
	      + ")\r\n\r\n");

  /* Okay, now keep entering data... */
  send_string("Next, please enter a one-line brief description.\r\n");
  send_string("Examples of brief descriptions:  "
	      + "'a sword', 'John', 'some bacon'.\r\n");

  substate = SS_PROMPT_BRIEF_DESC;

  /* The editor is going to print its own prompt, so don't bother
     with ours. */
  return RET_NORMAL;
}

static int prompt_brief_desc_input(string input) {
  object phr;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("That was all whitespace.  Let's try that again.\r\n");
    send_string("Please enter a one-line brief description.\r\n");
    send_string("Examples of brief descriptions:  "
		+ "'a sword', 'John', 'some bacon'.\r\n");

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);
  phr = new_obj->get_brief();
  phr->set_content_by_lang(get_user()->get_locale(), input);

  substate = SS_PROMPT_GLANCE_DESC;

  send_string("Please enter a one-line glance description.\r\n");
  send_string("Examples of brief descriptions:  "
	      + "'a red flashing toy gun', 'John the Butcher',"
	      + "or 'about a pound of bacon'.\r\n");

  return RET_NORMAL;
}

static int prompt_glance_desc_input(string input) {
  object edit_state, phr;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("That was all whitespace.  Let's try that again.\r\n");
    send_string("Please enter a one-line glance description.\r\n");
    send_string("Examples of brief descriptions:  "
		+ "'a red flashing toy gun', 'John the Butcher',"
		+ "or 'about a pound of bacon'.\r\n");

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);
  phr = new_obj->get_glance();
  phr->set_content_by_lang(get_user()->get_locale(), input);

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

  return RET_NO_PROMPT;
}

static void prompt_look_desc_data(mixed data) {
  object edit_state, phr;

  if(typeof(data) != T_STRING) {
    send_string("Non-string data passed to state!  Huh?  Cancelling.\r\n");
    pop_state();
    return;
  }

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

  data = STRINGD->trim_whitespace(data);
  phr = new_obj->get_look();
  phr->set_content_by_lang(get_user()->get_locale(), data);

  substate = SS_PROMPT_EXAMINE_DESC;
  send_string("\r\nLook desc accepted.\r\n");

  send_string("Now, enter a multiline 'examine' description.  This is what an"
	      + " observer would\r\n");
  send_string("note about this object with careful scrutiny.\r\n");
  send_string("Or hit '~' and enter and it will default to the look "
	      + "description.\r\n");

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
  string examine_desc;

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

  if(examine_desc && !STRINGD->is_whitespace(examine_desc)) {
    new_obj->set_examine(NEW_PHRASE(examine_desc));
  }
  substate = SS_PROMPT_NOUNS;

  send_string("\r\nOkay, now let's get a list of the nouns and adjectives "
	      + "you can\r\n");
  send_string("use to refer to the object.  For reference, we'll also let you"
	      + " see\r\n");
  send_string("the short descriptions you supplied.\r\n");
  send_string("Brief:  " + new_obj->get_brief()->to_string(get_user())
	      + "\r\n");
  send_string("Glance: " + new_obj->get_glance()->to_string(get_user())
	      + "\r\n");
  send_string("\r\nNow, give a space-separated list of nouns to refer to this"
	      + " object.\r\n");
  send_string("Example: sword blade hilt weapon pommel\r\n\r\n");

  /* Don't return anything, this is a void function */
}

/* Used when processing nouns and adjectives */
private mixed process_words(string input) {
  object  phr, location;
  string* words;
  int     ctr;

  words = explode(input, " ");

  for(ctr = 0; ctr < sizeof(words); ctr++) {
    words[ctr] = STRINGD->to_lower(words[ctr]);
  }

  phr = new_object(LWO_PHRASE);
  phr->set_content_by_lang(get_user()->get_locale(),
			   implode(words[0..],","));

  return phr;
}

static int prompt_nouns_input(string input) {
  string nouns;

  if(obj_type == OT_PORTABLE
     && (!input || STRINGD->is_whitespace(input))) {
    send_string("Nope.  You'll want at least one noun.  Try again.\r\n");
    send_string("Brief:  " + new_obj->get_brief()->to_string(get_user())
		+ "\r\n");
    send_string("Glance: " + new_obj->get_glance()->to_string(get_user())
		+ "\r\n");
    send_string("\r\nNow, give a space-separated list of nouns to refer to "
		+ "this object.\r\n");
    send_string("Example: sword blade weapon\r\n\r\n");
    return RET_NORMAL;
  }

  nouns = STRINGD->trim_whitespace(input);
  new_obj->add_noun(process_words(nouns));

  substate = SS_PROMPT_ADJECTIVES;

  send_string("\r\nGood.  Next, do the same for adjectives.\r\n");
  send_string("Example: heavy gray dull\r\n");

  return RET_NORMAL;
}

static int prompt_adjectives_input(string input) {
  object phr, edit_state;
  string adjectives;

  adjectives = STRINGD->trim_whitespace(input);
  if(adjectives && adjectives != "") {
    new_obj->add_adjective(process_words(adjectives));
  }

  if(obj_type == OT_ROOM) {
    send_string("\r\nDone with room #" + new_obj->get_number() + ".\r\n");
    return RET_POP_STATE;
  }

  substate = SS_PROMPT_CONTAINER;

  edit_state = clone_object(US_ENTER_YN);
  if(edit_state) {
    edit_state->set_prompt("Is the object a container? ");
    push_state(edit_state);
  } else {
    LOGD->write_syslog("Couldn't clone US_ENTER_YN state object!",
		       LOG_ERROR);
    return RET_POP_STATE;
  }

  return RET_NORMAL;
}

static void prompt_container_data(mixed data) {
  object edit_state;

  if(typeof(data) != T_INT) {
    send_string("Internal error -- wrong type passed!\r\n");
    pop_state();
    return;
  }

  if(!data) {
    /* Not a container, so neither open nor openable. */
    send_string("Done with portable #" + new_obj->get_number() + ".\r\n");
    pop_state();
    return;
  }

  new_obj->set_container(1);

  substate = SS_PROMPT_OPEN;

  edit_state = clone_object(US_ENTER_YN);
  if(edit_state) {
    edit_state->set_prompt("Is the container open? ");
    push_state(edit_state);
  } else {
    LOGD->write_syslog("Couldn't clone US_ENTER_YN state object!",
		       LOG_ERROR);
    pop_state();
    return;
  }
}

static void prompt_open_data(mixed data) {
  object edit_state;

  if(typeof(data) != T_INT) {
    send_string("Internal error -- wrong type passed!\r\n");
    pop_state();
    return;
  }

  if(data) {
    /* Container is open */
    new_obj->set_open(1);
  }

  substate = SS_PROMPT_OPENABLE;

  edit_state = clone_object(US_ENTER_YN);
  if(edit_state) {
    edit_state->set_prompt("Is the container freely openable and closeable? ");
    push_state(edit_state);
  } else {
    LOGD->write_syslog("Couldn't clone US_ENTER_YN state object!",
		       LOG_ERROR);
    pop_state();
    return;
  }
}

static void prompt_openable_data(mixed data) {
  if(typeof(data) != T_INT) {
    send_string("Internal error -- wrong type passed!\r\n");
    pop_state();
    return;
  }

  if(data) {
    new_obj->set_openable(1);
  }

  send_string("Done with portable #" + new_obj->get_number() + ".\r\n");

  pop_state();
}
