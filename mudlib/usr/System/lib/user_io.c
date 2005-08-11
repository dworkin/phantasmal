#include <kernel/kernel.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

/*
 * This library implements the user-side stuff for terminal
 * processing, user state stacks and similar tricks involving telnet,
 * MUD clients, and various forms of user interaction that doesn't
 * take place at the regular command prompt.
 */

inherit COMMON_AUTO;
inherit LIB_USER;

/* User-state processing stack */
private object* state_stack;

private object  scroll_state;

/* Terminal-type variables: */
private mapping substitutions;
private int     subs_correct;
private int     mudclient_conn;

/* Saved by save_object */
int    num_lines;               /* how many lines the terminal has */
int    num_cols;                /* how many columns the terminal has */
int    locale;                  /* chosen output locale */

/* Prototypes */
void   push_state(object state);
object peek_state(void);
void   to_state_stack(string str);
static nomask int send_string(string str);
void   upgraded(varargs int clone);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
    if(clone) {
      /* More defaults */
      num_lines = 20;
      num_cols = 78;

      /* Default to enUS locale */
      locale = PHRASED->language_by_name("english");

      state_stack = ({ });
    }

    upgraded(clone);
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    if(!find_object(US_SCROLL_TEXT)) { compile_object(US_SCROLL_TEXT); }
  } else
    error("Non-System code called upgraded!");
}


int get_num_lines(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code can call get_num_lines!");

  return num_lines;
}

static void set_num_lines(int new_num) {
  num_lines = new_num;
}

int get_locale(void) {
  return locale;
}

static void set_locale(int new_loc) {
  locale = new_loc;
}


/****** User Message Functions *****************/

int message(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(peek_state()) {
    to_state_stack(str);
  } else {
    return send_string(str);
  }
}

/*
 * NAME:	message_all_users()
 * DESCRIPTION:	send message to listening users
 */
static void message_all_users(string str)
{
    object *users, user;
    int i;

    if(!SYSTEM() && !COMMON() && !GAME())
      return;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
	user = users[i];
	if (user && user != this_object()) {
	    user->message(str);
	}
    }
}

/*
 * NAME:	system_phrase_all_users()
 * DESCRIPTION:	send message to listening users
 */
static void system_phrase_all_users(string str)
{
    object *users, user;
    int i;

    if(!SYSTEM() && !COMMON() && !GAME())
      return;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
	user = users[i];
	if (user != this_object()) {
	    user->send_system_phrase(str);
	}
    }
}

static string phrase_to_string(object PHRASE phrase) {
  string tmp;

  tmp = phrase->get_content_by_lang(locale);
  if(tmp) return tmp;

  tmp = phrase->get_content_by_lang(LANG_englishUS);
  if(tmp) return tmp;

  tmp = phrase->get_content_by_lang(LANG_debugUS);
  if(tmp) return tmp;

  return "(nil)";
}

/* This sends a Phrase, allowing locale and terminal settings to
   affect output */
int send_phrase(object PHRASE obj) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  return message(phrase_to_string(obj));
}

int send_system_phrase(string phrname) {
  object PHRASE phr;

  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  phr = PHRASED->file_phrase(SYSTEM_PHRASES, phrname);
  if(!phr) {
    LOGD->write_syslog("Can't find system phrase " + phrname + "!", LOG_ERR);
  }
  return send_phrase(phr);
}

/****** USER_STATE stack implementation ********/

/* This is called only by the USER_STATE object, and is used to send
   already-filtered output on the channel.
*/
nomask int ustate_send_string(string str) {
  if(previous_program() == USER_STATE)
    return ::message(str);
  else
    error("Only USER_STATE can call PHANTASMAL_USER:ustate_send_string!");
}

/* This does a lowest-level, unfiltered send to the connection object
   itself.  Normally sends will be filtered through the user_state
   object(s) active, if any, but this function is different. */
static nomask int send_string(string str) {
  return ::message(str);
}

void to_state_stack(string str) {
  if(SYSTEM() || COMMON() || GAME()) {
    state_stack[0]->to_user(str);
  }
}

void message_scroll(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code can call message_scroll!");

  if(scroll_state) {
    scroll_state->to_user(str);
  } else {
    scroll_state = clone_object(US_SCROLL_TEXT);
    if(scroll_state) {
      scroll_state->add_text(str);
      push_state(scroll_state);
    } else {
      LOGD->write_syslog("Couldn't clone US_SCROLL_TEXT state object!",
			 LOG_ERROR);
    }
  }
}

void notify_done_scrolling(void) {
  if(previous_object() != scroll_state)
    error("Only our own scrolling state can notify a user it's done!");

  scroll_state = nil;

}

void pop_state(object state) {
  int    first_state;  /* This is a boolean value */
  int    ctr;
  object prev_state, next_state;

  if(!SYSTEM() && !COMMON() && !GAME())
    return;

  if(!state_stack || !sizeof(state_stack)) {
    destruct_object(state);
    error("Popping empty stack!");
  }

  if(!(state_stack && ({ state })))
    error("Popping state not in stack!");

  if(state_stack[0] == state)
    first_state = 1;
  else {
    first_state = 0;
    for(ctr = 1; ; ctr++) {
      if(state_stack[ctr] == state) {
	prev_state = state_stack[ctr - 1];
	if(ctr + 1 < sizeof(state_stack))
	  next_state = state_stack[ctr + 1];
	break;
      }
    }
  }

  state_stack = state_stack - ({ state });

  if(first_state) {
    state->switch_from(1);  /* 1 because popp is true */
    if(sizeof(state_stack)) {
      state_stack[0]->switch_to(0);  /* 0 because pushp is false */
    }
    /* No longer print prompt here, that's handled in receive_message. */
  } else {
    /* Make sure that the next_state values are correct in user state. */
    prev_state->init(this_object(), next_state);
  }

  destruct_object(state);
}

void push_state(object state) {
  if(!SYSTEM() && previous_program() != USER_STATE)
    error("Only privileged code can call push_state()!");

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Unprivileged code calling SYSTEM_USER_IO:push_state!");

  if(!state_stack) {
    state_stack = ({ });
  }

  if(sizeof(state_stack)) {
    state->init(this_object(), state_stack[0]);
    state_stack[0]->switch_from(0);  /* 0 because popp is false */
  } else {
    state->init(this_object(), nil);
  }

  state_stack = ({ state }) + state_stack;

  /* Call switch_to() after adding the new state to the stack --
     'cause it can pop the state right back off in some cases... */
  state->switch_to(1); /* 1 because pushp is true */
}

void push_new_state(mixed state_type, mixed params...) {
  object new_state;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Unprivileged code calling SYSTEM_USER_IO:push_new_state!");

  new_state = clone_object(state_type);
  if(!new_state)
    error("Can't create new state to push in push_new_state()!");

  new_state->set_up_func(params...);
  push_state(new_state);
}

object peek_state(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code can call peek_state!");

  if(!state_stack || !sizeof(state_stack))
    return nil;

  return state_stack[0];
}

mixed state_receive_message(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  return state_stack[0]->from_user(str);
}

/****************/


/**********************************************/

/* UNQ markup stuff */

void set_up_substitutions(void) {
  object  conn;
  mapping info;

  if(subs_correct)
    return;

  conn = this_object()->query_conn();
  if(conn) {
    if(sscanf(object_name(conn), MUDCLIENT_CONN + "#%*d") == 1) {
      LOGD->write_syslog("Is MUDclient conn!");
      mudclient_conn = 1;
    } else {
      LOGD->write_syslog("Not MUDclient conn!");
      mudclient_conn = 0;
    }
  } else {
    return;  /* Can't set up subs yet */
  }

  if(!mudclient_conn) {
    /* This connection was made on a non-MUDclient port.  That means
       no term types, no ANSI color, no window size... */

    substitutions = ([ ]);

    subs_correct = 1;
    return;
  }

  info = conn->terminal_info();
  if(info["protocol"] == "telnet") {
    substitutions = ([ ]);
  } else if(info["protocol"] == "imp") {
    substitutions = ([
		      "client-startup" : "<IMPDEMO>",
		      "black" : "<FONT COLOR=\"black\"",
		      "/black" : "</FONT>",
		      "blue" : "<FONT COLOR=\"blue\"",
		      "/blue" : "</FONT>",
		      "green" : "<FONT COLOR=\"green\"",
		      "/green" : "</FONT>",
		      "cyan" : "<FONT COLOR=\"cyan\"",
		      "/cyan" : "</FONT>",
		      "red" : "<FONT COLOR=\"red\"",
		      "/red" : "</FONT>",
		      "purple" : "<FONT COLOR=\"purple\"",
		      "/purple" : "</FONT>",
		      ]);
  } else {
    error("Unrecognized protocol when setting up substitution maps!");
  }

  subs_correct = 1;
}

int supports_tag(string tag) {
  if(!subs_correct)
    set_up_substitutions();

  if(substitutions[tag])
    return 1;

  return 0;
}
