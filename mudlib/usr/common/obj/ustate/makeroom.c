#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>


inherit USER_STATE;

/* Vars for MAKEROOM user state */
private int    substate;

private object new_obj;

/* Data specified by user */
private int    obj_number;
private int    obj_type;
private object obj_detail_of;


/* Valid object-type values */
#define OT_UNKNOWN                  1
#define OT_ROOM                     2
#define OT_PORTABLE                 3
#define OT_DETAIL                   4


/* Valid substate values */
#define SS_PROMPT_OBJ_TYPE          1
#define SS_PROMPT_OBJ_DETAIL_OF     2
#define SS_PROMPT_OBJ_NUMBER        3
#define SS_PROMPT_OBJ_PARENT        4
#define SS_PROMPT_BRIEF_DESC        5
#define SS_PROMPT_LOOK_DESC         6
#define SS_PROMPT_EXAMINE_DESC      7
#define SS_PROMPT_NOUNS             8
#define SS_PROMPT_ADJECTIVES        9
#define SS_PROMPT_WEIGHT           10
#define SS_PROMPT_VOLUME           11
#define SS_PROMPT_LENGTH           12
/* Note the gap -- that so that if we add states and recompile, anybody
   who is mid-object-creation won't find themselves mysteriously being
   prompted for something else.  Now we only need to renumber *very*
   rarely. */
#define SS_PROMPT_CONTAINER        20
#define SS_PROMPT_OPEN             21
#define SS_PROMPT_OPENABLE         22
#define SS_PROMPT_WEIGHT_CAPACITY  23
#define SS_PROMPT_VOLUME_CAPACITY  24
#define SS_PROMPT_LENGTH_CAPACITY  25


/* Input function return values */
#define RET_NORMAL                  1
#define RET_POP_STATE               2
#define RET_NO_PROMPT               3


/* Prototypes */
static int  prompt_obj_type_input(string input);
static int  prompt_obj_number_input(string input);
static int  prompt_obj_detail_of_input(string input);
static int  prompt_obj_parent_input(string input);
static int  prompt_brief_desc_input(string input);
static void prompt_look_desc_data(mixed data);
static void prompt_examine_desc_data(mixed data);
static int  prompt_nouns_input(string input);
static int  prompt_adjectives_input(string input);
static int  prompt_weight_input(string input);
static int  prompt_volume_input(string input);
static int  prompt_length_input(string input);
static void prompt_container_data(mixed data);
static void prompt_open_data(mixed data);
static void prompt_openable_data(mixed data);
static int  prompt_weight_capacity_input(string input);
static int  prompt_volume_capacity_input(string input);
static int  prompt_length_capacity_input(string input);

private string blurb_for_substate(int substate);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)


static void create(varargs int clone) {
  ::create();
  if(!find_object(US_ENTER_DATA)) compile_object(US_ENTER_DATA);
  if(!find_object(US_ENTER_YN)) compile_object(US_ENTER_YN);
  if(!find_object(LWO_PHRASE)) compile_object(LWO_PHRASE);
  if(!find_object(SIMPLE_ROOM)) compile_object(SIMPLE_ROOM);
  if(clone) {
    substate = SS_PROMPT_OBJ_TYPE;
    obj_type = OT_UNKNOWN;
    obj_number = -1;
  }
}

private void specify_type(string type) {
  if(type == "room" || type == "r")
    obj_type = OT_ROOM;
  else if(type == "port" || type == "portable" || type == "p")
    obj_type = OT_PORTABLE;
  else if(type == "det" || type == "detail" || type == "d")
    obj_type = OT_DETAIL;
  else
    error("Illegal value supplied to specify_type!");

  if(obj_type == OT_DETAIL) {
    substate = SS_PROMPT_OBJ_DETAIL_OF;
  } else {
    substate = SS_PROMPT_OBJ_NUMBER;
  }
}

void set_up_func(varargs string new_type) {
  if(new_type)
    specify_type(new_type);
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
	send_string("(Quitting OLC -- not deleting obj #"
		    + new_obj->get_number() + " -- Cancelling!)\r\n");
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
  case SS_PROMPT_OBJ_DETAIL_OF:
    ret = prompt_obj_detail_of_input(input);
    break;
  case SS_PROMPT_OBJ_NUMBER:
    ret = prompt_obj_number_input(input);
    break;
  case SS_PROMPT_OBJ_PARENT:
    ret = prompt_obj_parent_input(input);
    break;
  case SS_PROMPT_BRIEF_DESC:
    ret = prompt_brief_desc_input(input);
    break;
  case SS_PROMPT_NOUNS:
    ret = prompt_nouns_input(input);
    break;
  case SS_PROMPT_ADJECTIVES:
    ret = prompt_adjectives_input(input);
    break;
  case SS_PROMPT_WEIGHT:
    ret = prompt_weight_input(input);
    break;
  case SS_PROMPT_VOLUME:
    ret = prompt_volume_input(input);
    break;
  case SS_PROMPT_LENGTH:
    ret = prompt_length_input(input);
    break;
  case SS_PROMPT_WEIGHT_CAPACITY:
    ret = prompt_weight_capacity_input(input);
    break;
  case SS_PROMPT_VOLUME_CAPACITY:
    ret = prompt_volume_capacity_input(input);
    break;
  case SS_PROMPT_LENGTH_CAPACITY:
    ret = prompt_length_capacity_input(input);
    break;

  case SS_PROMPT_LOOK_DESC:
  case SS_PROMPT_EXAMINE_DESC:
  case SS_PROMPT_CONTAINER:
  case SS_PROMPT_OPEN:
  case SS_PROMPT_OPENABLE:
    send_string("Internal error in state machine!  Cancelling OLC!\r\n");
    LOGD->write_syslog("Reached from_user() in state " + substate
		       + " while doing @make_room.  Illegal!", LOG_ERR);
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


/* This is if somebody (other than us) is sending data to the user.
   This happens if, for instance, somebody moves into or out of the
   same room and the player sees a message. */
void to_user(string output) {
  /* For the moment, suspend output to the player who is mid-OLC */
  /* send_string(output); */
}


/* This function returns the text to send to the player in the given
   substate at the given moment.  Since we do a lot of "that was
   illegal, let's try it again" type prompting, it gets messy to have
   the text every place the player can screw something up.  The
   supplied substate is assumed to be the one the player is currently
   in, and the prompt is assumed to be prior to input (or prior
   to re-entering input after unacceptable input). */
private string blurb_for_substate(int substate) {
  string tmp;

  switch(substate) {

  case SS_PROMPT_OBJ_TYPE:
    return "Please enter an object type or 'quit' to quit.\r\n"
      + "Valid values are:  room, portable, detail (r/p/d)\r\n";

  case SS_PROMPT_OBJ_NUMBER:
    return "Enter the desired object number for this object\r\n"
      + "  or hit enter to assign it automatically.\r\n";

  case SS_PROMPT_OBJ_DETAIL_OF:
    return "Enter the base object number for this detail or type 'quit'.\r\n"
      + "That's the existing object that this is a detail of.\r\n";

  case SS_PROMPT_OBJ_PARENT:
    return "Enter the object's parents for data inheritance.\r\n"
      + "Example: #37 #247 #1343\r\n"
      + "You can also hit enter for no parents, or type 'quit' to quit.\r\n"
      + "Parents are like Skotos ur-objects (see help @set_obj_parent).\r\n";

  case SS_PROMPT_BRIEF_DESC:
    return "Next, please enter a one-line brief description.\r\n"
      + "Examples of brief descriptions:  "
      + "'a sword', 'John', 'some bacon'.\r\n";

  case SS_PROMPT_LOOK_DESC:
    return "Now, enter a multiline 'look' description.  This is what an"
      + " observer would note\r\n"
      + "specifically about this object on quick perusal.\r\n";

  case SS_PROMPT_EXAMINE_DESC:
    return "Now, enter a multiline 'examine' description.  This is what an"
      + " observer would\r\n"
      + "note about this object with careful scrutiny.\r\n"
      + "Or hit '~' and enter and it will default to the look "
      + "description.\r\n";

  case SS_PROMPT_NOUNS:
    tmp = "Brief:  " + new_obj->get_brief()->to_string(get_user()) + "\r\n";

    if(sizeof(new_obj->get_archetypes())) {
      tmp += "Parent nouns: ";
      tmp += implode(new_obj->get_nouns(get_user()->get_locale()), ", ");
      tmp += "\r\n";
    }

    tmp += "\r\nGive a space-separated list of new nouns to refer to this"
      + " object.\r\n"
      + "Example: sword blade hilt weapon pommel\r\n\r\n";

    return tmp;

  case SS_PROMPT_ADJECTIVES:
    tmp = "Brief:  " + new_obj->get_brief()->to_string(get_user()) + "\r\n";

    if(sizeof(new_obj->get_archetypes())) {
      tmp += "Parent adjectives: ";
      tmp += implode(new_obj->get_adjectives(get_user()->get_locale()), ", ");
      tmp += "\r\n";
    }

    tmp += "Enter a space-separate list of new adjectives to refer to this"
      + " object.\r\n"
      + "Example: heavy metallic red\r\n";

    return tmp;

  case SS_PROMPT_WEIGHT:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Enter the weight of the object or type 'none' to default to"
	+ " the parent value.\r\n"
	+ "The weight may be in kilograms, or may be followed by a "
	+ "unit.\r\n"
	+ "Metric: mg   g   kg     Standard: lb   oz   tons\r\n";
    return "Enter the weight of the object.\r\n"
      + "The weight may be in kilograms, or may be followed by a "
      + "unit.\r\n"
      + "Metric: mg   g   kg     Standard: lb   oz   tons\r\n";

  case SS_PROMPT_VOLUME:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Enter the volume of the object or 'none' to default to"
	+ "\r\n  the parent value.\r\n"
	+ "The volume may be in liters, or may be followed by a "
	+ "unit.\r\n"
	+ "Metric: L   mL   cc   cubic m\r\n"
	+ "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";
    return "Enter the volume of the object.\r\n"
      + "The volume may be in liters, or may be followed by a "
      + "unit.\r\n"
      + "Metric: L   mL   cc   cubic m\r\n"
      + "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";

  case SS_PROMPT_LENGTH:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Enter the length of the longest axis of the object, or type\r\n"
	+ "  'none' to default to the parent value.\r\n"
	+ "The length may be in centimeters, or may be followed by a "
	+ "unit.\r\n"
	+ "Metric: m   mm   cm   dm     Standard: in   ft   yd\r\n";
    return "Enter the length of the longest axis of the object.\r\n"
      + "The length may be in centimeters, or may be followed by a "
      + "unit.\r\n"
      + "Metric: m   mm   cm   dm     Standard: in   ft   yd\r\n";

    /* The following don't have full-on blurbs, just one-line prompts
       for the ENTER_YN user state.  So no blurb. */
  case SS_PROMPT_CONTAINER:
  case SS_PROMPT_OPEN:
  case SS_PROMPT_OPENABLE:
    return "This blurb should never be used.  Oops!\r\n";

  case SS_PROMPT_WEIGHT_CAPACITY:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Enter the weight capacity of the object or 'none' to default"
	+ " to\r\n  the parent's value.\r\n"
	+ "The capacity may be in kilograms, or may be followed by a "
	+ "unit.\r\n"
	+ "Metric: mg   g   kg     Standard: lb   oz   tons\r\n";
    return "Enter the weight capacity of the object.\r\n"
      + "The capacity may be in kilograms, or may be followed by a "
      + "unit.\r\n"
      + "Metric: mg   g   kg     Standard: lb   oz   tons\r\n";

  case SS_PROMPT_VOLUME_CAPACITY:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Enter the volume capacity of the object, or type 'none' to "
	+ " default\r\n  to the parent's value.\r\n"
	+ "The capacity may be in liters, or may be followed by a "
	+ "unit.\r\n"
	+ "Metric: L   mL   cc   cubic m\r\n"
	+ "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";
    return "Enter the volume capacity of the object\r\n"
      + "The capacity may be in liters, or may be followed by a "
      + "unit.\r\n"
      + "Metric: L   mL   cc   cubic m\r\n"
      + "Standard: oz   qt   gal   cubic ft   cubic yd\r\n";

  case SS_PROMPT_LENGTH_CAPACITY:
    if(new_obj && sizeof(new_obj->get_archetypes()))
      return "Enter the length of the longest axis of the container, or type "
	+ " 'none' to\r\n  default to the parent value.\r\n"
	+ "The length may be in centimeters, or may be followed by a "
	+ "unit.\r\n"
	+ "Metric: m   mm   cm   dm     Standard: in   ft   yd\r\n";
    return "Enter the length of the longest axis of the container.\r\n"
      + "The length may be in centimeters, or may be followed by a "
      + "unit.\r\n"
      + "Metric: m   mm   cm   dm     Standard: in   ft   yd\r\n";

  default:
    return "<UNDEFINED STATE>\r\n";
  }
}


/* This is called when the state is switched to.  The pushp parameter
   describes whether a push or a pop switched to this state -- if
   pushp is true, the state was just allocated and started.  If it's
   false, a state got pushed and we resumed control afterward. */
void switch_to(int pushp) {
  if(pushp
     && (substate == SS_PROMPT_OBJ_NUMBER
	 || substate == SS_PROMPT_OBJ_TYPE
	 || substate == SS_PROMPT_OBJ_DETAIL_OF)) {
    /* Just allocated */
    send_string("Creating a new object.  Type 'quit' at the prompt"
		+ " (except on multiline prompts) to cancel.\r\n");
    send_string(blurb_for_substate(substate));
    send_string(" > ");
  } else if (substate == SS_PROMPT_LOOK_DESC
	     || substate == SS_PROMPT_EXAMINE_DESC) {
    /* Do nothing */
  } else if (substate == SS_PROMPT_NOUNS
	     || substate == SS_PROMPT_WEIGHT_CAPACITY) {
    /* This means we just got back from getting a desc */
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

  /* TODO:  we should probably use the binder for this */
  if(!input
     || (input != "r" && input != "p" && input != "d"
	 && input != "room"
	 && input != "port" && input != "portable"
	 && input != "det" && input != "detail")) {
    send_string("That's not a valid object type.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_OBJ_TYPE));

    return RET_NORMAL;
  }

  if(input[0] == "r"[0]) {
    obj_type = OT_ROOM;
  } else if(input[0] == "d"[0]) {
    obj_type = OT_DETAIL;
  } else {
    obj_type = OT_PORTABLE;
  }

  if(obj_type == OT_DETAIL) {
    substate = SS_PROMPT_OBJ_DETAIL_OF;
  } else {
    substate = SS_PROMPT_OBJ_NUMBER;
  }

  send_string(blurb_for_substate(substate));

  /* The editor is going to print its own prompt, so don't bother
     with ours. */
  return RET_NORMAL;
}

static int prompt_obj_detail_of_input(string input) {
  int base_num;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("\r\nYou have to specify a base object."
		+ "  Let's try again.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  if(sscanf(input, "%*d %*s") == 2
     || sscanf(input, "%*s %*d") == 2
     || sscanf(input, "%d", base_num) != 1) {
    send_string("\r\nYou need to supply a single object number.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  if(base_num < 1) {
    send_string("\r\nObject numbers need to be greater than zero.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  obj_detail_of = MAPD->get_room_by_num(base_num);
  if(!obj_detail_of) {
    send_string("\r\nThere doesn't seem to be a room or portable #"
		+ base_num + ".\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  send_string("\r\nBase object accepted.\r\n");
  substate = SS_PROMPT_OBJ_NUMBER;
  send_string(blurb_for_substate(substate));

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
      send_string("Please *only* enter a number.\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));
      return RET_NORMAL;
    }
    /* Object number was parsed. */
    if(obj_number < 1) {
      send_string("That doesn't appear to be a legal object number.\r\n");
      send_string("Object numbers should be positive and nonzero.\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));

      return RET_NORMAL;
    }
    if(MAPD->get_room_by_num(obj_number)) {
      send_string("There is already an object #" + obj_number + ".\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));

      return RET_NORMAL;
    }
    segown = OBJNUMD->get_segment_owner(obj_number / 100);
    if(obj_number >= 0 && segown && segown != MAPD) {
      user->message("Object #" + obj_number
		    + " is in a segment somebody owned by "
		    + segown + "!\r\n");
      send_string(blurb_for_substate(SS_PROMPT_OBJ_NUMBER));

      return RET_NORMAL;
    }

    /* Okay, object number looks good -- continue. */
  }

  if(obj_type == OT_DETAIL) {
    location = obj_detail_of;
  } else {
    location = get_user()->get_location();
    if(location && obj_type == OT_ROOM) {
      /* The new room should be put into the same place as the room
	 the user is currently standing in.  Makes a good default. */
      location = location->get_location();
    }
  }

  /* Rooms, portables and details are now all cloned from the same
     base. */
  new_obj = clone_object(SIMPLE_ROOM);

  if(!new_obj) {
    send_string("Sorry, you seem to be out of objects or memory!\r\n");

    return RET_POP_STATE;
  }

  zonenum = -1;
  if(obj_number < 0) {
    /* Get zone based on object type and location */
    if(obj_type == OT_DETAIL) {
      zonenum = ZONED->get_zone_for_room(obj_detail_of);
    } else if(get_user()->get_location()) {
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

  if(obj_type == OT_DETAIL
     && !obj_detail_of) {
    send_string("Somebody has deleted the base object between the time you"
		+ " entered it\r\n  and now.  No detail was created."
		+ "  Exiting OLC!\r\n");
    destruct_object(new_obj);
    return RET_POP_STATE;
  }

  if(obj_detail_of) {
    obj_detail_of->add_detail(new_obj);
  } else if(location) {
    location->add_to_container(new_obj);
  }

  send_string("Added obj #" + new_obj->get_number()
	      + " to zone #" + zonenum
	      + " (" + ZONED->get_name_for_zone(zonenum) + ")" + ".\r\n");
  if(obj_detail_of) {
    send_string("It is a detail of obj ");
  } else {
    send_string("Its location is ");
  }
  if(location) {
    string tmp;

    if(location->get_brief()) {
      tmp = location->get_brief()->to_string(get_user());
    } else {
      tmp = "(undescribed)";
    }

    send_string("#" + location->get_number() + "(" + tmp + ")\r\n\r\n");
  } else {
    send_string("nowhere\r\n\r\n");
  }

  /* Okay, now keep entering data... */
  substate = SS_PROMPT_OBJ_PARENT;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_obj_parent_input(string input) {
  int     parnum, ctr;
  object *obj_parents;
  object  obj_parent;
  string *parent_strings;

  if(!input || STRINGD->is_whitespace(input)) {
    /* No parent -- that works. */

    substate = SS_PROMPT_BRIEF_DESC;
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  parent_strings = explode(input, " ");
  for(ctr = 0; ctr < sizeof(parent_strings); ctr++) {
    if(parent_strings[ctr] == "#")
      continue;

    if((!sscanf(parent_strings[ctr], "#%d", parnum)
	&& !sscanf(parent_strings[ctr], "%d", parnum))
       || (parnum < 0)) {
      send_string("Each parent must be a valid positive integer, or a"
		  + " positive integer\r\n  following a # sign.\r\n");
      send_string("'" + parent_strings[ctr] + "' is not.\r\n");
      send_string(blurb_for_substate(substate));
      return RET_NORMAL;
    }

    if(!(obj_parent = MAPD->get_room_by_num(parnum))) {
      send_string("There is no valid object #" + parnum + ".\r\n");
      send_string(blurb_for_substate(substate));
      return RET_NORMAL;
    }

    obj_parents += ({ obj_parent });
  }

  if(obj_parents && sizeof(obj_parents)) {
    new_obj->set_archetypes(obj_parents);
  } else {
    send_string("Internal error.  Weird.  Try again.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  substate = SS_PROMPT_BRIEF_DESC;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}

static int prompt_brief_desc_input(string input) {
  object PHRASE phr;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("That was all whitespace.  Let's try that again.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_BRIEF_DESC));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);
  phr = new_obj->get_brief();
  phr->set_content_by_lang(get_user()->get_locale(), input);

  substate = SS_PROMPT_LOOK_DESC;

  send_string(blurb_for_substate(substate));
  push_new_state(US_ENTER_DATA);

  return RET_NORMAL;
}

static void prompt_look_desc_data(mixed data) {
  object PHRASE phr;

  if(typeof(data) != T_STRING) {
    send_string("Non-string data passed to state!  Huh?  Cancelling.\r\n");
    pop_state();
    return;
  }

  if(!data || STRINGD->is_whitespace(data)) {
    send_string("That look description was all whitespace.  "
		+ "Let's try that again.\r\n");
    send_string(blurb_for_substate(SS_PROMPT_LOOK_DESC));

    push_new_state(US_ENTER_DATA);

    return;
  }

  data = STRINGD->trim_whitespace(data);
  phr = new_obj->get_look();
  phr->set_content_by_lang(get_user()->get_locale(), data);

  substate = SS_PROMPT_EXAMINE_DESC;
  send_string("\r\nLook desc accepted.\r\n");

  send_string(blurb_for_substate(SS_PROMPT_EXAMINE_DESC));

  push_new_state(US_ENTER_DATA);
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
	      + "you can\r\n"
	      + "use to refer to the object.  For reference, we'll also"
	      + " let you see\r\n"
	      + "the short descriptions you supplied.\r\n");
  send_string(blurb_for_substate(SS_PROMPT_NOUNS));

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
    send_string(blurb_for_substate(SS_PROMPT_NOUNS));
    return RET_NORMAL;
  }

  nouns = STRINGD->trim_whitespace(input);
  new_obj->add_noun(process_words(nouns));

  substate = SS_PROMPT_ADJECTIVES;

  send_string("Good.  Now do the same for adjectives.\r\n");
  send_string(blurb_for_substate(SS_PROMPT_ADJECTIVES));

  return RET_NORMAL;
}

static int prompt_adjectives_input(string input) {
  object PHRASE phr;
  string adjectives;

  adjectives = STRINGD->trim_whitespace(input);
  if(adjectives && adjectives != "") {
    new_obj->add_adjective(process_words(adjectives));
  }

  if(obj_type == OT_ROOM) {
    new_obj->set_container(1);
    new_obj->set_open(1);

    new_obj->set_weight(2000.0);    /* 2 metric tons */
    new_obj->set_volume(1000000.0); /* Equiv of 10m cubic room */
    new_obj->set_length(1000.0);    /* 10m -- too big to pick up */
    new_obj->set_weight_capacity(2000000.0);   /* 2000 metric tons */
    new_obj->set_volume_capacity(27000000.0);  /* Equiv of 30m cubic room */
    new_obj->set_length_capacity(1500.0);      /* 15m */

    send_string("\r\nDone with room #" + new_obj->get_number() + ".\r\n");
    return RET_POP_STATE;
  }

  if(obj_type == OT_PORTABLE) {
    substate = SS_PROMPT_WEIGHT;

    send_string("Good.  Now we'll get the object's weight.\r\n");
    send_string(blurb_for_substate(substate));
    return RET_NORMAL;
  }

  /* If it's not a portable or a room, it's a detail.  In that case,
     don't bother with the weight, volume and length for this object.
     It's part of another.  It *will* have capacities later, though,
     if it's a container. */

  substate = SS_PROMPT_CONTAINER;

  push_new_state(US_ENTER_YN, "Is the object a container? ");

  return RET_NORMAL;
}


static int prompt_weight_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Let's try that again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "kg"             : 1.0,
	       "kilograms"      : 1.0,
	       "kilogram"       : 1.0,
	       "g"              : 0.001,
	       "grams"          : 0.001,
	       "gram"           : 0.001,
	       "mg"             : 0.000001,
	       "milligrams"     : 0.000001,
	       "milligram"      : 0.000001,
	       "pounds"         : 0.45,
	       "pound"          : 0.45,
	       "lb"             : 0.45,
	       "ounces"         : 0.028,
	       "ounce"          : 0.028,
	       "oz"             : 0.028,
	       "tons"           : 990.0,
	       "ton"            : 990.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("I don't recognize the units '" + unitstr
		  + "' as units of weight or mass.  Try again.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Enter the value, optionally with units.  Something like:\r\n"
		+ "  '4.7 oz' or '3 tons' or '0.5 mg'.  Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("You'll want to give a number no smaller than zero.  "
		+ "Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_weight(value);

  send_string("Accepted weight.\r\n");
  substate = SS_PROMPT_VOLUME;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_volume_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Let's try that again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "liter"             : 1.0,
               "liters"            : 1.0,
               "l"                 : 1.0,
               "L"                 : 1.0,
               "milliliter"        : 0.001,
               "milliliters"       : 0.001,
               "ml"                : 0.001,
               "mL"                : 0.001,
               "cubic centimeters" : 0.001,
               "cubic centimeter"  : 0.001,
	       "cc"                : 0.001,
	       "cubic cm"          : 0.001,
	       "cu cm"             : 0.001,
	       "cubic meters"      : 1000.0,
	       "cubic meter"       : 1000.0,
	       "cubic m"           : 1000.0,
	       "cu m"              : 1000.0,

	       "ounces"            : 0.0296,
	       "ounce"             : 0.0296,
	       "oz"                : 0.0296,
	       "pints"             : 0.473,
	       "pint"              : 0.473,
	       "pt"                : 0.473,
	       "quarts"            : 0.946,
	       "quart"             : 0.946,
	       "qt"                : 0.946,
	       "gallons"           : 3.784,
	       "gallon"            : 3.784,
	       "gal"               : 3.784,
	       "cubic foot"        : 28.3,
	       "cubic feet"        : 28.3,
	       "cubic ft"          : 28.3,
	       "cu ft"             : 28.3,
	       "cubic yards"       : 765.0,
	       "cubic yard"        : 765.0,
	       "cubic yd"          : 765.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("I don't recognize the units '" + unitstr
		  + "' as units of volume.  Try again.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Enter the value, optionally with units.  Something like:\r\n"
		+ "  '1.2 liters' or '250 cc' or '35 cu dm'.  Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("You'll want to give a number no smaller than zero.  "
		+ "Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_volume(value);

  send_string("Accepted volume.\r\n");
  substate = SS_PROMPT_LENGTH;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_length_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Let's try that again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "centimeters"    : 1.0,
	       "centimeter"     : 1.0,
	       "cm"             : 1.0,
	       "millimeters"    : 0.1,
	       "millimeter"     : 0.1,
	       "mm"             : 0.1,
	       "decimeters"     : 10.0,
	       "decimeter"      : 10.0,
	       "dm"             : 10.0,
	       "meters"         : 100.0,
	       "meter"          : 100.0,
	       "m"              : 100.0,

	       "inches"         : 2.54,
	       "inch"           : 2.54,
	       "in"             : 2.54,
	       "feet"           : 30.5,
	       "foot"           : 30.5,
	       "ft"             : 30.5,
	       "yards"          : 91.4,
	       "yard"           : 91.4,
	       "yd"             : 91.4,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("I don't recognize the units '" + unitstr
		  + "' as units of length.  Try again.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Enter the value, optionally with units.  Something like:\r\n"
		+ "  '4.7 oz' or '3 tons' or '0.5 mg'.  Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("You'll want to give a number no smaller than zero.  "
		+ "Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_length(value);

  send_string("Accepted length.\r\n");
  substate = SS_PROMPT_CONTAINER;

  push_new_state(US_ENTER_YN, "Is the object a container? ");

  return RET_NORMAL;
}


static void prompt_container_data(mixed data) {
  if(typeof(data) != T_INT) {
    send_string("Internal error -- wrong type passed!\r\n");
    pop_state();
    return;
  }

  if(!data) {
    /* Not a container, so neither open nor openable. */
    if(obj_type == OT_PORTABLE) {
      send_string("Done with portable #" + new_obj->get_number() + ".\r\n");
    } else {
      send_string("Done with detail #" + new_obj->get_number() + ".\r\n");
    }
    pop_state();
    return;
  }

  new_obj->set_container(1);

  substate = SS_PROMPT_OPEN;

  push_new_state(US_ENTER_YN, "Is the container open? ");
}

static void prompt_open_data(mixed data) {
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

  push_new_state(US_ENTER_YN,
		 "Is the container freely openable and closeable? ");
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

  substate = SS_PROMPT_WEIGHT_CAPACITY;
  send_string(blurb_for_substate(substate));
}


static int prompt_weight_capacity_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Let's try that again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "kg"             : 1.0,
	       "kilograms"      : 1.0,
	       "kilogram"       : 1.0,
	       "g"              : 0.001,
	       "grams"          : 0.001,
	       "gram"           : 0.001,
	       "mg"             : 0.000001,
	       "milligrams"     : 0.000001,
	       "milligram"      : 0.000001,
	       "pounds"         : 0.45,
	       "pound"          : 0.45,
	       "lb"             : 0.45,
	       "ounces"         : 0.028,
	       "ounce"          : 0.028,
	       "oz"             : 0.028,
	       "tons"           : 990.0,
	       "ton"            : 990.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("I don't recognize the units '" + unitstr
		  + "' as units of weight or mass.  Try again.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Enter the value, optionally with units.  Something like:\r\n"
		+ "  '4.7 oz' or '3 tons' or '0.5 mg'.  Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("You'll want to give a number no smaller than zero.  "
		+ "Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_weight_capacity(value);

  send_string("Accepted weight capacity.\r\n");
  substate = SS_PROMPT_VOLUME_CAPACITY;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_volume_capacity_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Let's try that again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "liter"             : 1.0,
               "liters"            : 1.0,
               "l"                 : 1.0,
               "L"                 : 1.0,
               "milliliter"        : 0.001,
               "milliliters"       : 0.001,
               "ml"                : 0.001,
               "mL"                : 0.001,
               "cubic centimeters" : 0.001,
               "cubic centimeter"  : 0.001,
	       "cc"                : 0.001,
	       "cubic cm"          : 0.001,
	       "cu cm"             : 0.001,
	       "cubic meters"      : 1000.0,
	       "cubic meter"       : 1000.0,
	       "cubic m"           : 1000.0,
	       "cu m"              : 1000.0,

	       "ounces"            : 0.0296,
	       "ounce"             : 0.0296,
	       "oz"                : 0.0296,
	       "pints"             : 0.473,
	       "pint"              : 0.473,
	       "pt"                : 0.473,
	       "quarts"            : 0.946,
	       "quart"             : 0.946,
	       "qt"                : 0.946,
	       "gallons"           : 3.784,
	       "gallon"            : 3.784,
	       "gal"               : 3.784,
	       "cubic foot"        : 28.3,
	       "cubic feet"        : 28.3,
	       "cubic ft"          : 28.3,
	       "cu ft"             : 28.3,
	       "cubic yards"       : 765.0,
	       "cubic yard"        : 765.0,
	       "cubic yd"          : 765.0,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("I don't recognize the units '" + unitstr
		  + "' as units of volume.  Try again.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Enter the value, optionally with units.  Something like:\r\n"
		+ "  '1.2 liters' or '250 cc' or '35 cu dm'.  Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("You'll want to give a number no smaller than zero.  "
		+ "Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_volume_capacity(value);

  send_string("Accepted volume capacity.\r\n");
  substate = SS_PROMPT_LENGTH_CAPACITY;
  send_string(blurb_for_substate(substate));

  return RET_NORMAL;
}


static int prompt_length_capacity_input(string input) {
  mapping units;
  string  unitstr;
  float   value;
  int     use_parent;

  use_parent = 0;

  if(!input || STRINGD->is_whitespace(input)) {
    send_string("Let's try that again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  input = STRINGD->trim_whitespace(input);

  if(sscanf(input, "%f %s", value, unitstr) == 2) {
    units = ([ "centimeters"    : 1.0,
	       "centimeter"     : 1.0,
	       "cm"             : 1.0,
	       "millimeters"    : 0.1,
	       "millimeter"     : 0.1,
	       "mm"             : 0.1,
	       "decimeters"     : 10.0,
	       "decimeter"      : 10.0,
	       "dm"             : 10.0,
	       "meters"         : 100.0,
	       "meter"          : 100.0,
	       "m"              : 100.0,

	       "inches"         : 2.54,
	       "inch"           : 2.54,
	       "in"             : 2.54,
	       "feet"           : 30.5,
	       "foot"           : 30.5,
	       "ft"             : 30.5,
	       "yards"          : 91.4,
	       "yard"           : 91.4,
	       "yd"             : 91.4,
	       ]);

    unitstr = STRINGD->trim_whitespace(unitstr);
    if(units[unitstr])
      value *= units[unitstr];
    else {
      send_string("I don't recognize the units '" + unitstr
		  + "' as units of length.  Try again.\r\n");
      send_string(blurb_for_substate(substate));

      return RET_NORMAL;
    }
  } else if(!STRINGD->stricmp(input, "none")) {
    use_parent = 1;
    value = -1.0;
  } else if(sscanf(input, "%f", value) != 1) {
    send_string("Enter the value, optionally with units.  Something like:\r\n"
		+ "  '4.7 oz' or '3 tons' or '0.5 mg'.  Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  if(value < 0.0 && !use_parent) {
    send_string("You'll want to give a number no smaller than zero.  "
		+ "Try again.\r\n");
    send_string(blurb_for_substate(substate));

    return RET_NORMAL;
  }

  new_obj->set_length_capacity(value);

  send_string("Accepted length capacity.\r\n\r\n");
  substate = SS_PROMPT_CONTAINER;

  if(obj_type == OT_PORTABLE) {
    send_string("Done with portable #" + new_obj->get_number() + ".\r\n");
  } else {
    send_string("Done with detail #" + new_obj->get_number() + ".\r\n");
  }

  return RET_POP_STATE;
}
